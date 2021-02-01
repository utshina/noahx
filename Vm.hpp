#pragma once

#include "Flag.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <WinHvPlatform.h>

class Vcpu;

class Vm
{
public:
	typedef uint64_t gpa_t;
	enum prot_t : uint64_t {
		PROT_NONE = 0,
		PROT_READ = WHvMapGpaRangeFlagRead,
		PROT_WRITE = WHvMapGpaRangeFlagWrite,
		PROT_EXEC = WHvMapGpaRangeFlagExecute,
	};

private:
	static const int MAX_VCPU = 8;

	WHV_PARTITION_HANDLE m_handle;
	std::array<std::unique_ptr<Vcpu>, MAX_VCPU> m_vcpu;

public:
	// CREATORS
	Vm();
	~Vm();

	// MANIPULATORS
	void
	map(gpa_t gpa, void *hva, size_t size, prot_t prot);

	Vcpu&
	createVcpu();

	void
	deleteVcpu(UINT32 id);

	// ACCESSORS
	const WHV_PARTITION_HANDLE
	getHandle() { return m_handle; }
};

ENUM_FLAG(Vm::prot_t);
