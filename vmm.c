#include <stdio.h>
#include <stdnoreturn.h>
#include <windows.h>
#include <WinHvPlatform.h>
#include "vmm.h"

#define countof(a) (sizeof(a)/sizeof(a[0]))

#define SUCCESS 0x80000000
#define OR & SUCCESS ^ SUCCESS ||

noreturn static int
panic(char *msg)
{
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		      FORMAT_MESSAGE_FROM_SYSTEM | 
		      FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL, GetLastError(),
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		      (LPTSTR) &lpMsgBuf, 0, NULL);
	fprintf(stderr, (const char *)lpMsgBuf);
	exit(1);
}

static inline guest_physical_t
kernel_hvirt_to_gphys(vm_t *vm, void *hvirt)
{
	return (guest_physical_t)hvirt - (guest_physical_t)vm->kernel
		+ vm->kernel_start_guest_physical;
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
vmm_setup_vm(vm_t *vm)
{
	vm->text_guest_physical = 1 GiB;
	vm->text_size = 1 GiB;
	vm->kernel_start_guest_physical = 255 GiB;
	vm->kernel_size = 1 MiB;
	printf("kernel start: %llx\n", vm->kernel_start_guest_physical);

	vm->text = aligned_alloc(PAGE_SIZE, vm->text_size);
	if (!vm->text)
		panic("out of memory");
	vm->text[0] = 0xeb;
	vm->text[1] = 0xfe;
//	page[0] = 0x0f; page[1] = 0x34;

	WHvMapGpaRange(
		vm->handle, vm->text,
		vm->text_guest_physical, vm->text_size,
		WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite | WHvMapGpaRangeFlagExecute)
		OR panic("map gpa range error (user)");

	vm->kernel = aligned_alloc(PAGE_SIZE, vm->kernel_size);
	if (!vm->kernel)
		panic("out of memory");

	WHvMapGpaRange(
		vm->handle, vm->kernel,
		vm->kernel_start_guest_physical, vm->kernel_size,
		WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite)
		OR panic("map gpa range error (kernel)");

	memset(vm->kernel, 0, sizeof(*vm->kernel));
	uint64_t *pml4_entry = (uint64_t *)vm->kernel->pml4;
	uint64_t *pdpt_entry = (uint64_t *)vm->kernel->pdpt;
	pml4_entry[0] = kernel_hvirt_to_gphys(vm, pdpt_entry) | 0x07; // P & RW & US
	pdpt_entry[0] = 0;
	pdpt_entry[1] = 1 GiB | 0x87; // P & RW & US & PS & PGE

	vm->kernel->gdt[0] = 0;
	vm->kernel->gdt[1] = 0x00a0fa000000ffff; // user code
	vm->kernel->gdt[2] = 0x00c0f2000000ffff; // user data
}

void
vmm_create_vcpu(vm_t *vm)
{
int i;

	vm->vcpu_id = 0;
	WHvCreateVirtualProcessor(vm->handle, vm->vcpu_id, 0)
		OR panic("create virtual processor error");

	enum regs {
		Cr0, Cr3, Cr4, Efer, Ldtr, Idtr, Gdtr, Tr, Cs, Rip, Ss, Rsp,
		Ds, Es, Fs, Gs,
	};

	WHV_REGISTER_NAME regname[] = {
#define REGNAME(reg) WHvX64Register##reg
		[Cr0] = REGNAME(Cr0),
		[Cr3] = REGNAME(Cr3),
		[Cr4] = REGNAME(Cr4),
		[Efer] = REGNAME(Efer),
		[Idtr] = REGNAME(Idtr),
		[Gdtr] = REGNAME(Gdtr),
		[Tr] = REGNAME(Tr),
		[Cs] = REGNAME(Cs),
		[Rip] = REGNAME(Rip),
		[Ss] = REGNAME(Ss),
		[Rsp] = REGNAME(Rsp),
		[Ds] = REGNAME(Ds),
		[Es] = REGNAME(Es),
		[Fs] = REGNAME(Fs),
		[Gs] = REGNAME(Gs),
	};
	WHV_REGISTER_VALUE regvalue[countof(regname)];
	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regname, countof(regname), regvalue)
		OR panic("get virtual processor registers error");
	for (i = 0; i < countof(regname); i++)
		printf("%d: %llx\n", i, regvalue[i].Reg64);

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
	regvalue[Rip].Reg64 = vm->text_guest_physical;
	WHV_X64_SEGMENT_REGISTER DataSegment = {
		.Base = 0, .Limit = 0xffff,
		.Selector = 0x10, .Attributes = 0xc0f3,
	};
	regvalue[Rsp].Reg64 = 2 GiB;
	regvalue[Ss].Segment = DataSegment;
	regvalue[Ds].Segment = DataSegment;
	regvalue[Es].Segment = DataSegment;
	regvalue[Fs].Segment = DataSegment;
	regvalue[Gs].Segment = DataSegment;
	for (i = 0; i < countof(regname); i++)
		printf("%d: %llx\n", i, regvalue[i].Reg64);
	WHvSetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regname, countof(regname), regvalue)
		OR panic("set virtual processor registers error");

	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regname, countof(regname), regvalue)
		OR panic("get virtual processor registers error");
	for (i = 0; i < countof(regname); i++)
		printf("%d: %llx\n", i, regvalue[i].Reg64);
}

bool
vmm_run(vm_t *vm)
{
	WHvRunVirtualProcessor(
		vm->handle, vm->vcpu_id,
		&vm->exit_context, sizeof(vm->exit_context))
		OR panic("run virtual processor error");
}
