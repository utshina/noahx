#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vmm.h"

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
	printf("state: %x\n", vm.exit_context.VpContext.ExecutionState.AsUINT16);
	printf("cs: %x, %llx\n", vm.exit_context.VpContext.Cs.Selector, vm.exit_context.VpContext.Cs.Attributes);
	printf("rip: %llx\n", vm.exit_context.VpContext.Rip);
	switch(vm.exit_context.ExitReason) {
	case WHvRunVpExitReasonMemoryAccess:
		printf("gva: %llx\n", vm.exit_context.MemoryAccess.Gva);
		printf("access: %x\n", vm.exit_context.MemoryAccess.AccessInfo.AsUINT32);
		break;

	case WHvRunVpExitReasonUnrecoverableException:
		printf("error code: %d\n", vm.exit_context.VpException.ErrorCode);
		printf("inst count: %d\n", vm.exit_context.VpException.InstructionByteCount);

	default:
	;
	}
}
