#include "Vm.hpp"
#include "Vcpu.hpp"
#include "panicw.hpp"
#include "Debug.hpp"
#include <iostream>
#include <windows.h>

static void
init()
{
	UINT32 size;
	WHV_CAPABILITY capability;

	WHvGetCapability(
		WHvCapabilityCodeHypervisorPresent,
		&capability, sizeof(capability), &size);
	if (!capability.HypervisorPresent)
		panic("Windows Hypervisor Platform is not enabled");
}

Vm::Vm()
{
	[[maybe_unused]] static bool initialized = []() { init(); return true; }();

	WHvCreatePartition(&m_handle)
		OR panic("create partition error");

	UINT32 cpu_count = MAX_VCPU;
	WHvSetPartitionProperty(
		m_handle,
		WHvPartitionPropertyCodeProcessorCount,
		&cpu_count, sizeof(cpu_count))
		OR panic("set partition property error");

	WHvSetupPartition(m_handle)
		OR panic("setup partition error");

	LOG("create\n");
}

Vm::~Vm()
{
	for (auto&& e : m_vcpu)
		e = nullptr;
	WHvDeletePartition(m_handle)
		OR panic("delete partition error");
	LOG("delete\n");
}

void
Vm::map(gpa_t gpa, void *hva, size_t size, prot_t prot)
{
	HRESULT hr;

	WHV_MAP_GPA_RANGE_FLAGS wprot = static_cast<WHV_MAP_GPA_RANGE_FLAGS>(prot);
	hr = WHvMapGpaRange(m_handle, hva, gpa, size, wprot);
	if (FAILED(hr))
		panicw(hr, "user mmap error");
	LOG(std::hex, "map: gpa=", gpa, " hva", hva, " size=", size, " prot=", prot);
}

Vcpu&
Vm::createVcpu()
{
	for (size_t i = 0; i < m_vcpu.size(); i++) {
		if (!m_vcpu[i]) {
			m_vcpu[i] = std::make_unique<Vcpu>(*this, i);
			return *m_vcpu[i];
		}
	}
	panic("too many VCPU");
}

void
Vm::deleteVcpu(UINT32 id)
{
	m_vcpu[id] = nullptr;
}