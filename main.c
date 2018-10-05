#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <unistd.h>
#include <windows.h>
#include <WinHvPlatform.h>
#include "regs.h"

#define countof(a) (sizeof(a)/sizeof(a[0]))

#define SUCCESS 0x80000000
#define OR & SUCCESS ^ SUCCESS ||

noreturn static int
panic(char *msg)
{
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	exit(1); 
}

typedef struct {
	WHV_PARTITION_HANDLE handle;
	UINT32 vcpu_id;
	WHV_RUN_VP_EXIT_CONTEXT exit_context;
} vm_t;

const WHV_GUEST_PHYSICAL_ADDRESS gva = 0x1000;

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

static void
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

static void
vmm_setup_vm(vm_t *vm)
{
	const long page_size = sysconf(_SC_PAGESIZE);
	char *page = aligned_alloc(page_size, page_size);
	if (!page)
		panic("out of memory");
	page[0] = 0xeb; page[1] = 0xfe;
//	page[0] = 0x0f; page[1] = 0x34;

	WHvMapGpaRange(
		vm->handle, page, gva, page_size,
		WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagExecute)
		OR panic("map gpa range error");
}

static void
vmm_create_vcpu(vm_t *vm)
{
	vm->vcpu_id = 0;
	WHvCreateVirtualProcessor(vm->handle, vm->vcpu_id, 0)
		OR panic("create virtual processor error");

	WHV_REGISTER_NAME regs[] = {
		WHvX64RegisterCr4,
		WHvX64RegisterEfer,
		WHvX64RegisterCs,
		WHvX64RegisterRip,
	};
	WHV_REGISTER_VALUE vals[countof(regs)];
	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regs, countof(regs), vals)
		OR panic("get virtual processor registers error");

	vals[0].Reg64 = CR4_PAE;
	vals[1].Reg64 = EFER_LME | EFER_LMA;
	vals[2].Reg64 = 0;
	vals[3].Reg64 = gva;
	printf("efer: %llx\n", vals[1].Reg64);
	WHvSetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regs, countof(regs), vals)
		OR panic("set virtual processor registers error");

	WHvGetVirtualProcessorRegisters(
		vm->handle, vm->vcpu_id, regs, countof(regs), vals)
		OR panic("get virtual processor registers error");
	printf("efer: %llx\n", vals[1].Reg64);
}

static bool
vmm_run(vm_t *vm)
{
	WHvRunVirtualProcessor(
		vm->handle, vm->vcpu_id,
		&vm->exit_context, sizeof(vm->exit_context))
		OR panic("run virtual processor error");
}

int main()
{
	vm_t vm;

	printf("Starting Noahx ...\n");

	vmm_init();
	vmm_create_vm(&vm);
	vmm_setup_vm(&vm);
	vmm_create_vcpu(&vm);
	vmm_run(&vm);

	printf("exit reason: %x\n", vm.exit_context.ExitReason);
	printf("gva: %llx\n", vm.exit_context.MemoryAccess.Gva);
	printf("access: %x\n", vm.exit_context.MemoryAccess.AccessInfo.AsUINT32);
}
