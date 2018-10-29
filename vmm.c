#include <stdio.h>
#include <stdnoreturn.h>
#include <alloca.h>
#include <windows.h>
#include "vmm.h"

#define countof(a) (sizeof(a)/sizeof(a[0]))

#define SUCCESS 0x80000000
#define OR & SUCCESS ^ SUCCESS ?:

noreturn static void
panic(char *msg)
{
	fprintf(stderr, "vmm: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

noreturn static void
panicw(HRESULT result, char *msg)
{
	fprintf(stderr, "vmm: ");
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

void
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
vmm_create_vm(vm_t *vm)
{
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
vmm_mmap(vm_t *vm, void *hvirt, vmm_gphys_t gphys, size_t size, vmm_mmap_prot_t prot)
{
	HRESULT hr;

	printf("mmap: hvirt=%p, gphys=%llx, size=%llx, prot=%x\n",
	       hvirt, gphys, size, prot);
	hr = WHvMapGpaRange(vm->handle, hvirt, gphys, size, prot);
	if (FAILED(hr))
		panicw(hr, "user mmap error");
}

char *p;

void
vmm_setup_vm(vm_t *vm)
{
	static const uint64_t KERNEL_OFFSET = 255 GiB;
	vm->stack_top_gphys = KERNEL_OFFSET;
	vm->stack_size = 1 GiB;
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

void
vmm_push(vm_t *vm, const void *data, size_t n)
{
	uint64_t remainder = roundup(n, 8) - n;
	void *sp = stack_gphys_to_hvirt(vm, vm->regs.rsp);
	memcpy(sp, data, n);
	memset(sp + n, 0, remainder);
	vm->regs.rsp -= n;
}

void
vmm_create_vcpu(vm_t *vm)
{
int i;

	vm->vcpu_id = 0;
	WHvCreateVirtualProcessor(vm->handle, vm->vcpu_id, 0)
		OR panic("create virtual processor error");

	enum regs {
		Cr0 = 0, Cr3, Cr4, Efer, Idtr, Gdtr, Tr,
		Cs, Rip, Ss, Rsp, Rbp, Ds, Es, Fs, Gs,
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
		REGNAME_ENTRY(Rip),
		REGNAME_ENTRY(Ss),
		REGNAME_ENTRY(Rsp),
		REGNAME_ENTRY(Rbp),
		REGNAME_ENTRY(Ds),
		REGNAME_ENTRY(Es),
		REGNAME_ENTRY(Fs),
		REGNAME_ENTRY(Gs),
	};
	WHV_REGISTER_VALUE regvalue[countof(regname)];
	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regname, countof(regname), regvalue)
		OR panic("get virtual processor registers error");

	regvalue[Cr0].Reg64 |= (CR0_PE | CR0_PG);
	regvalue[Cr3].Reg64 = kernel_hvirt_to_gphys(vm, vm->kernel->pml4);
	regvalue[Cr4].Reg64 |= (CR4_PSE | CR4_PAE);
	regvalue[Efer].Reg64 |= (EFER_LME | EFER_LMA);
	regvalue[Idtr].Table.Base = kernel_hvirt_to_gphys(vm, vm->kernel->idt);
	regvalue[Idtr].Table.Limit = 8 * 256 - 1;
	regvalue[Gdtr].Table.Base = kernel_hvirt_to_gphys(vm, vm->kernel->gdt);
	regvalue[Gdtr].Table.Limit = 0x17;
	regvalue[Tr].Segment.Base = kernel_hvirt_to_gphys(vm, vm->kernel->tss);
	regvalue[Tr].Segment.Limit = 0xffff;
	regvalue[Tr].Segment.Selector = 0x10;
	regvalue[Tr].Segment.Attributes = 0x808b;
	WHV_X64_SEGMENT_REGISTER CodeSegment = {
		.Base = 0, .Limit = 0xffff,
		.Selector = 0x08, .Attributes = 0xa0fb,
	};
	regvalue[Cs].Segment = CodeSegment;
	regvalue[Rip].Reg64 = vm->regs.rip;
	printf("%llx\n", regvalue[Rip].Reg64);
	regvalue[Rsp].Reg64 = vm->regs.rsp;
	regvalue[Rbp].Reg64 = vm->regs.rsp;
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
		vm->handle, vm->vcpu_id, regname, countof(regname), regvalue)
		OR panic("set virtual processor registers error");
}

void
vmm_get_regs(vm_t *vm, vmm_regs_t *regs, int count)
{
	WHV_REGISTER_NAME *regname = alloca(sizeof(*regname) * count);
	WHV_REGISTER_VALUE *regvalue = alloca(sizeof(*regvalue) * count);
	for (int i = 0; i < count; i++)
		regname[i] = regs[i].name;
	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regname, count, regvalue)
		OR panic("get virtual processor registers error");
	for (int i = 0; i < count; i++)
		regs[i].value = regvalue[i];
}

bool
vmm_run(vm_t *vm)
{
	WHvRunVirtualProcessor(
		vm->handle, vm->vcpu_id,
		&vm->exit_context, sizeof(vm->exit_context))
		OR panic("run virtual processor error");
}
