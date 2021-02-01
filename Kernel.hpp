#pragma once

#include "Vm.hpp"
#include "Common.hpp"

class Kernel
{
private:
	Vm& m_vm;
	Vm::gpa_t m_start;
	size_t m_size;
	static const int PAGE_ENTRY_COUNT = 512;
	struct kernel {
		uint64_t pml4[PAGE_ENTRY_COUNT];
		uint64_t pdpt[PAGE_ENTRY_COUNT];
		char tss[PAGE_SIZE_4K];
		uint64_t gdt[3];
	} *m_data;

public:
	Kernel(Vm& vm) : m_vm(vm) {};
	~Kernel();

	void
	setup();

	Vm::gpa_t
	hva_to_gpa(void *hva);
};
