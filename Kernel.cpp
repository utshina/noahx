#include "Kernel.hpp"
#include "Common.hpp"
#include "panic.h"

inline Vm::gpa_t
Kernel::hva_to_gpa(void *hva)
{
	uint64_t offset = reinterpret_cast<uint64_t>(hva) - reinterpret_cast<uint64_t>(m_data);
	return m_start + offset;
}

void
Kernel::setup()
{
	m_start = 255 * GiB;
	m_size = roundup(sizeof(*m_data), PAGE_SIZE_4K);
	m_data = static_cast<struct kernel *>(aligned_alloc(PAGE_SIZE_4K, m_size));
	if (m_data)
		panic("out of memory");
	memset(m_data, 0, m_size);
	m_vm.map(m_start, m_data, m_size, Vm::PROT_READ | Vm::PROT_WRITE);

	constexpr uint64_t PTE_P  = 1ULL << 0;
	constexpr uint64_t PTE_RW = 1ULL << 1;
	constexpr uint64_t PTE_US = 1ULL << 2;
	constexpr uint64_t PTE_PS = 1ULL << 7;
	constexpr uint64_t PML4E_FLAG = PTE_P | PTE_RW | PTE_US;
	constexpr uint64_t PDPTE_FLAG = PTE_P | PTE_RW | PTE_US | PTE_PS;
	m_data->pml4[0] = hva_to_gpa(m_data->pdpt) | PML4E_FLAG;
	const uint64_t pdpte_count = m_start / (1 * GiB);
	for (uint64_t i = 0; i < pdpte_count; i++)
		m_data->pdpt[i] = (i * GiB) | PDPTE_FLAG;

	m_data->gdt[0] = 0;
	m_data->gdt[1] = 0x00a0fa000000ffff; // user code
	m_data->gdt[2] = 0x00c0f2000000ffff; // user data
}

