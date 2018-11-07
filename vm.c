#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <alloca.h>
#include "panic.h"
#include "vm.h"

#define SUCCESS 0x80000000
#undef OR
#define OR & SUCCESS ^ SUCCESS ?:

static bool initialized = false;

noreturn void
panicw(HRESULT result, char *msg)
{
	fprintf(stderr, "vm: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM |
		      FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL, result,
		      MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		      (LPTSTR) &lpMsgBuf, 0, NULL);
	fprintf(stderr, (const char *)lpMsgBuf);
	exit(EXIT_FAILURE);
}

static void
vmm_init(void)
{
	UINT32 size;
	WHV_CAPABILITY capability;

	WHvGetCapability(
		WHvCapabilityCodeHypervisorPresent,
		&capability, sizeof(capability), &size);
	if (!capability.HypervisorPresent)
		panic("Windows Hypervisor Platform is not enabled");
}

void
vm_create(vm_t *vm)
{
	if (!initialized) {
		vmm_init();
		initialized = true;
	}

	assert(vm != NULL);
	WHvCreatePartition(&vm->handle)
		OR panic("create partition error");

	UINT32 cpu_count = 1;
	WHvSetPartitionProperty(
		vm->handle,
		WHvPartitionPropertyCodeProcessorCount,
		&cpu_count, sizeof(cpu_count))
		OR panic("set partition property error");

	WHvSetupPartition(vm->handle)
		OR panic("setup partition error");
}

void
vm_mmap(vm_t *vm, vm_gphys_t gphys, void *hvirt, size_t size, vm_mmap_prot_t prot)
{
	HRESULT hr;

	assert(vm != NULL);
	printf("mmap: hvirt=%p, gphys=%lx, size=%lx, prot=%x\n",
		hvirt, gphys, size, prot);
	hr = WHvMapGpaRange(vm->handle, hvirt, gphys, size, prot);
	if (FAILED(hr))
		panicw(hr, "user mmap error");
}

#if 0
void
vmm_setup_vm(vm_t *vm)
{
	static const uint64_t KERNEL_OFFSET = 255 GiB;
	vm->stack_top_gphys = KERNEL_OFFSET;
	vm->stack_size = 1 MiB;
	vm->kernel_gphys = KERNEL_OFFSET;
	vm->kernel_size = 1 MiB;

	vm->stack = aligned_alloc(PAGE_SIZE_4K, vm->stack_size);
	if (!vm->stack)
		panic("out of memory");
	vmm_mmap(vm, vm->stack,
		 vm->stack_top_gphys - vm->stack_size, vm->stack_size,
		 VMM_MMAP_PROT_READ | VMM_MMAP_PROT_WRITE);
	vm->regs.rsp = vm->stack_top_gphys;

	vm->heap_gvirt = 0;

	vm->kernel = aligned_alloc(PAGE_SIZE_4K, vm->kernel_size);
	if (!vm->kernel)
		panic("out of memory");
	vmm_mmap(vm, vm->kernel, vm->kernel_gphys, vm->kernel_size,
		 VMM_MMAP_PROT_READ | VMM_MMAP_PROT_WRITE);

	memset(vm->kernel, 0, sizeof(*vm->kernel));
	uint64_t *pml4_entry = (uint64_t *)vm->kernel->pml4;
	uint64_t *pdpt_entry = (uint64_t *)vm->kernel->pdpt;
	// PML4 entry = P & RW & US
	pml4_entry[0] = kernel_hvirt_to_gphys(vm, pdpt_entry) | 0x07;
	for (int i = 0; i < KERNEL_OFFSET / (1 GiB); i++)
		pdpt_entry[i] = i GiB | 0x87; // P & RW & US & PS & PGE

	vm->kernel->gdt[0] = 0;
	vm->kernel->gdt[1] = 0x00a0fa000000ffff; // user code
	vm->kernel->gdt[2] = 0x00c0f2000000ffff; // user data
}
#endif

void
vm_create_vcpu(vm_t *vm, vcpu_t *vcpu)
{
	static UINT16 vcpu_id = 0;

	vcpu->vm = vm;
	vcpu->id = vcpu_id++;
	WHvCreateVirtualProcessor(vm->handle, vcpu->id, 0)
		OR panic("create virtual processor error");
}

void
vcpu_init_sysregs(vcpu_t *vcpu, vcpu_sysregs_t *sysregs)
{
	vm_t *vm = vcpu->vm;

	enum {
		Cr0, Cr3, Cr4, Efer, Gdtr, Idtr, Tr,
		Cs, Ss, Ds, Es, Fs, Gs,
	};
	WHV_REGISTER_NAME regname[] = {
#define REGNAME_ENTRY(reg) [reg] = WHvX64Register##reg
		REGNAME_ENTRY(Cr0),
		REGNAME_ENTRY(Cr3),
		REGNAME_ENTRY(Cr4),
		REGNAME_ENTRY(Efer),
		REGNAME_ENTRY(Idtr),
		REGNAME_ENTRY(Gdtr),
		REGNAME_ENTRY(Tr),
		REGNAME_ENTRY(Cs),
		REGNAME_ENTRY(Ss),
		REGNAME_ENTRY(Ds),
		REGNAME_ENTRY(Es),
		REGNAME_ENTRY(Fs),
		REGNAME_ENTRY(Gs),
	};
	WHV_REGISTER_VALUE regvalue[countof(regname)];
	WHvGetVirtualProcessorRegisters(
		vm->handle, vcpu->id, regname, countof(regname), regvalue)
		OR panic("get virtual processor registers error");

	static const uint64_t CR0_PE = 1ULL << 0;
	static const uint64_t CR0_PG = 1ULL << 31;
	static const uint64_t CR4_PSE = 1ULL << 4;
	static const uint64_t CR4_PAE = 1ULL << 5;
	static const uint64_t CR4_PGE = 1ULL << 7;
	static const uint64_t CR4_OSFXSR = 1ULL << 9;
	static const uint64_t CR4_OSXMMEXCPT = 1ULL << 10;
	static const uint64_t EFER_LME = 1ULL << 8;
	static const uint64_t EFER_LMA = 1ULL << 10;

	regvalue[Cr0].Reg64 |= (CR0_PE | CR0_PG);
	regvalue[Cr3].Reg64 = sysregs->cr3;
	regvalue[Cr4].Reg64 |= (CR4_PSE | CR4_PAE | CR4_PGE | CR4_OSFXSR | CR4_OSXMMEXCPT);
	regvalue[Efer].Reg64 |= (EFER_LME | EFER_LMA);
	regvalue[Gdtr].Table.Base = sysregs->gdt_base;
	regvalue[Gdtr].Table.Limit = sysregs->gdt_limit;
	regvalue[Idtr].Table.Base = sysregs->idt_base;
	regvalue[Idtr].Table.Limit = sysregs->idt_limit;
	regvalue[Tr].Segment.Base = sysregs->tss_base;
	regvalue[Tr].Segment.Limit = sysregs->tss_limit;
	regvalue[Tr].Segment.Selector = 0x10;
	regvalue[Tr].Segment.Attributes = 0x808b;
	WHV_X64_SEGMENT_REGISTER CodeSegment = {
		.Base = 0, .Limit = 0xffff,
		.Selector = 0x08, .Attributes = 0xa0fb,
	};
	regvalue[Cs].Segment = CodeSegment;
	WHV_X64_SEGMENT_REGISTER DataSegment = {
		.Base = 0, .Limit = 0xffff,
		.Selector = 0x10, .Attributes = 0xc0f3,
	};
	regvalue[Ss].Segment = DataSegment;
	regvalue[Ds].Segment = DataSegment;
	regvalue[Es].Segment = DataSegment;
	regvalue[Fs].Segment = DataSegment;
	regvalue[Gs].Segment = DataSegment;
	WHvSetVirtualProcessorRegisters(
		vm->handle, vcpu->id, regname, countof(regname), regvalue)
		OR panic("set virtual processor registers error");
}

void
vcpu_get_regs(vcpu_t *vcpu, vcpu_regs_t *regs, int count)
{
	WHV_REGISTER_NAME regname[sizeof(WHV_REGISTER_NAME) * count];
	WHV_REGISTER_VALUE regvalue[sizeof(WHV_REGISTER_VALUE) * count];

	for (int i = 0; i < count; i++)
		regname[i] = regs[i].name;
	WHvGetVirtualProcessorRegisters(
		vcpu->vm->handle, vcpu->id, regname, count, regvalue)
		OR panic("get virtual processor registers error");
	for (int i = 0; i < count; i++)
		*regs[i].ptr = regvalue[i].Reg64;
}

void
vcpu_set_regs(vcpu_t *vcpu, vcpu_regs_t *regs, int count)
{
	WHV_REGISTER_NAME regname[sizeof(WHV_REGISTER_NAME) * count];
	WHV_REGISTER_VALUE regvalue[sizeof(WHV_REGISTER_VALUE) * count];

	for (int i = 0; i < count; i++)
		regname[i] = regs[i].name;
	for (int i = 0; i < count; i++)
		regvalue[i].Reg64 = regs[i].value;
	WHvSetVirtualProcessorRegisters(
		vcpu->vm->handle, vcpu->id, regname, count, regvalue)
		OR panic("set virtual processor registers error");
}

void
vcpu_run(vcpu_t *vcpu)
{
	vm_t *vm = vcpu->vm;
	WHvRunVirtualProcessor(
		vm->handle, vcpu->id,
		&vm->exit_context, sizeof(vm->exit_context))
		OR panic("run virtual processor error");
}
