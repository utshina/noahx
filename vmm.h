#pragma once

#include <stdbool.h>
#include "WinHvPlatform.h"
#include "range.h"
#include "util.h"

#define KiB *1L*1024
#define MiB *1L*1024*1024
#define GiB *1L*1024*1024*1024
#define PAGE_SIZE_4K 4096

typedef uint64_t vmm_gvirt_t;
typedef uint64_t vmm_gphys_t;

typedef struct {
	range_t range;
	char *hvirt;
	uint64_t size;
} vmm_mmap_range_t;

typedef struct {
	bool in_operation;
	WHV_PARTITION_HANDLE handle;
	WHV_RUN_VP_EXIT_CONTEXT exit_context;

	struct {
		uint64_t rip;
		uint64_t rsp;
	} regs;

	vmm_mmap_range_t *mmap_range_root;

	char *stack;
	vmm_gphys_t stack_top_gphys;
	uint64_t stack_size;

	char *heap;
	vmm_gvirt_t heap_gvirt;

	struct kernel {
		char pml4[PAGE_SIZE_4K];
		char pdpt[PAGE_SIZE_4K];
		char tss[PAGE_SIZE_4K];
		char idt[PAGE_SIZE_4K];
		uint64_t gdt[3];
	} *kernel;
	vmm_gphys_t kernel_gphys;
	uint64_t kernel_size;

	UINT32 vcpu_id;
} vm_t;

#define VMM_VM_

void
vmm_init(void);

void
vmm_create_vm(vm_t *vm);

typedef enum {
	VMM_MMAP_PROT_READ = WHvMapGpaRangeFlagRead,
	VMM_MMAP_PROT_WRITE = WHvMapGpaRangeFlagWrite,
	VMM_MMAP_PROT_EXEC = WHvMapGpaRangeFlagExecute,

	VMM_MMAP_PROT_RE = VMM_MMAP_PROT_READ | VMM_MMAP_PROT_EXEC,
	VMM_MMAP_PROT_RW = VMM_MMAP_PROT_READ | VMM_MMAP_PROT_WRITE,
	VMM_MMAP_PROT_RWE = VMM_MMAP_PROT_READ | VMM_MMAP_PROT_WRITE | VMM_MMAP_PROT_EXEC,
} vmm_mmap_prot_t;

void
vmm_mmap(vm_t *vm, void *hvirt, vmm_gphys_t gphys, size_t size, vmm_mmap_prot_t prot);

void *
vmm_gvirt_to_hvirt(vm_t *vm, vmm_gvirt_t gvirt);

void
vmm_setup_vm(vm_t *vm);

void
vmm_push(vm_t *vm, const void *data, size_t n);

void
vmm_create_vcpu(vm_t *vm);

void
vmm_run(vm_t *vm);

enum {
	VMM_REGS_RAX = WHvX64RegisterRax,
	VMM_REGS_RCX = WHvX64RegisterRcx,
	VMM_REGS_RDX = WHvX64RegisterRdx,
	VMM_REGS_RBX = WHvX64RegisterRbx,
	VMM_REGS_RSP = WHvX64RegisterRsp,
	VMM_REGS_RSI = WHvX64RegisterRsi,
	VMM_REGS_RDI = WHvX64RegisterRdi,
	VMM_REGS_R8 = WHvX64RegisterR8,
	VMM_REGS_R9 = WHvX64RegisterR9,
	VMM_REGS_R10 = WHvX64RegisterR10,
	VMM_REGS_R11 = WHvX64RegisterR11,
	VMM_REGS_R12 = WHvX64RegisterR12,
	VMM_REGS_R13 = WHvX64RegisterR13,
	VMM_REGS_R14 = WHvX64RegisterR14,
	VMM_REGS_R15 = WHvX64RegisterR15,
	VMM_REGS_RIP = WHvX64RegisterRip,
	VMM_REGS_RFLAGS = WHvX64RegisterRflags,
};
typedef struct {
	WHV_REGISTER_NAME name;
	WHV_REGISTER_VALUE value;
} vmm_regs_t;
#define REGS_ENTRY_GET(reg) [reg] = { .name = VMM_REGS_##reg }
#define REGS_ENTRY_SET(reg, val) [reg] = { .name = VMM_REGS_##reg, .value = val }

void
vmm_get_regs(vm_t *vm, vmm_regs_t *regs, int count);

void
vmm_set_regs(vm_t *vm, vmm_regs_t *regs, int count);


static inline vmm_gphys_t
kernel_hvirt_to_gphys(vm_t *vm, void *hvirt)
{
	return (uint64_t)hvirt - (uint64_t)vm->kernel + vm->kernel_gphys;
}
static inline void *
stack_gphys_to_hvirt(vm_t *vm, vmm_gphys_t gphys)
{
	vmm_gphys_t stack_bottom_gphys = vm->stack_top_gphys - vm->stack_size;
	return gphys - stack_bottom_gphys + vm->stack;
}
static inline vmm_gphys_t
stack_hvirt_to_gphys(vm_t *vm, void *hvirt)
{
	return (uint64_t)hvirt - (uint64_t)vm->stack + vm->stack_top_gphys - vm->stack_size;
}

static inline uint64_t
rounddown(uint64_t value, size_t size)
{
	return (value & (~(size - 1)));
}

static inline uint64_t
roundup(uint64_t value, size_t size)
{
	return rounddown(value + size - 1, size);
}

static const uint64_t CR0_PE = 1ULL << 0;
static const uint64_t CR0_PG = 1ULL << 31;

static const uint64_t CR4_PSE = 1ULL << 4;
static const uint64_t CR4_PAE = 1ULL << 5;
static const uint64_t CR4_PGE = 1ULL << 7;

static const uint64_t EFER_LME = 1ULL << 8;
static const uint64_t EFER_LMA = 1ULL << 10;
