#pragma once

#include <stdbool.h>
#include <WinHvPlatform.h>
#include "range.h"
#include "util.h"

typedef uint64_t vm_gphys_t;

typedef struct {
	WHV_PARTITION_HANDLE handle;
	WHV_RUN_VP_EXIT_CONTEXT exit_context;
} vm_t;
void
vm_create(vm_t *vm);

typedef int vm_mmap_prot_t;
static const vm_mmap_prot_t VM_MMAP_PROT_READ = WHvMapGpaRangeFlagRead;
static const vm_mmap_prot_t VM_MMAP_PROT_WRITE = WHvMapGpaRangeFlagWrite;
static const vm_mmap_prot_t VM_MMAP_PROT_EXEC = WHvMapGpaRangeFlagExecute;

void
vm_mmap(vm_t *vm, vm_gphys_t gphys, void *hvirt, size_t size, vm_mmap_prot_t prot);

typedef struct {
	bool in_operation;
	vm_t *vm;
	UINT32 id;
} vcpu_t;
void
vm_create_vcpu(vm_t *vm, vcpu_t *vcpu);

typedef struct {
	uint64_t cr3;
	uint64_t gdt_base;
	uint16_t gdt_limit;
	uint64_t idt_base;
	uint16_t idt_limit;
	uint64_t tss_base;
	uint16_t tss_limit;

} vcpu_sysregs_t;
void
vcpu_init_sysregs(vcpu_t *vcpu, vcpu_sysregs_t *sysregs);

void
vcpu_run(vcpu_t *vcpu);

typedef enum {
	VCPU_REG_RAX = WHvX64RegisterRax,
	VCPU_REG_RCX = WHvX64RegisterRcx,
	VCPU_REG_RDX = WHvX64RegisterRdx,
	VCPU_REG_RBX = WHvX64RegisterRbx,
	VCPU_REG_RSP = WHvX64RegisterRsp,
	VCPU_REG_RBP = WHvX64RegisterRbp,
	VCPU_REG_RSI = WHvX64RegisterRsi,
	VCPU_REG_RDI = WHvX64RegisterRdi,
	VCPU_REG_R8 = WHvX64RegisterR8,
	VCPU_REG_R9 = WHvX64RegisterR9,
	VCPU_REG_R10 = WHvX64RegisterR10,
	VCPU_REG_R11 = WHvX64RegisterR11,
	VCPU_REG_R12 = WHvX64RegisterR12,
	VCPU_REG_R13 = WHvX64RegisterR13,
	VCPU_REG_R14 = WHvX64RegisterR14,
	VCPU_REG_R15 = WHvX64RegisterR15,
	VCPU_REG_RIP = WHvX64RegisterRip,
	VCPU_REG_RFLAGS = WHvX64RegisterRflags,

	VCPU_SEG_FS = WHvX64RegisterFs,
	VCPU_SEG_GS = WHvX64RegisterGs,
} vcpu_regname_t;
typedef uint64_t vcpu_regvalue_t;
typedef struct {
	vcpu_regname_t name;
	union {
		vcpu_regvalue_t value;
		vcpu_regvalue_t *ptr;
	};
} vcpu_regs_t;
#define VCPU_REGS_ENTRY_GET(reg, val) { VCPU_REG_##reg, { (vcpu_regvalue_t)val } }
#define VCPU_REGS_ENTRY_SET(reg, val) { VCPU_REG_##reg, { val } }

void
vcpu_get_regs(vcpu_t *vcpu, vcpu_regs_t *regs, int count);
void
vcpu_set_regs(vcpu_t *vcpu, vcpu_regs_t *regs, int count);
void
vcpu_set_segbase(vcpu_t *vcpu, vcpu_regname_t seg, uint64_t base);

#if 0
static inline void *
stack_gphys_to_hvirt(vm_t *vm, vmm_gphys_t gphys)
{
	vmm_gphys_t stack_bottom_gphys = vm->stack_top_gphys - vm->stack_size;
	return gphys - stack_bottom_gphys + vm->stack;
}
#endif

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

