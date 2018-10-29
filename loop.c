#include <stdio.h>
#include "vmm.h"

void
handle_vmexit(vm_t *vm)
{
	printf("exit reason: %x\n", vm->exit_context.ExitReason);
	printf("state: %llx\n", vm->exit_context.VpContext.ExecutionState.AsUINT16);
	printf("cs: %lx\n", vm->exit_context.VpContext.Cs.Selector);
	printf("rip: %llx\n", vm->exit_context.VpContext.Rip);

	switch(vm->exit_context.ExitReason) {
	case WHvRunVpExitReasonMemoryAccess:
		printf("inst: %02x %02x %02x %02x\n",
		       vm->exit_context.MemoryAccess.InstructionBytes[0],
		       vm->exit_context.MemoryAccess.InstructionBytes[1],
		       vm->exit_context.MemoryAccess.InstructionBytes[2],
		       vm->exit_context.MemoryAccess.InstructionBytes[3]
			);
		printf("access info: %x\n", vm->exit_context.MemoryAccess.AccessInfo.AsUINT32);
		printf("gpa: %llx\n", vm->exit_context.MemoryAccess.Gpa);
		printf("gva: %llx\n", vm->exit_context.MemoryAccess.Gva);
		break;

	case WHvRunVpExitReasonUnrecoverableException:
		printf("error code: %d\n", vm->exit_context.VpException.ErrorCode);
		printf("inst count: %d\n", vm->exit_context.VpException.InstructionByteCount);

	default:
		;
	}

	vm->in_operation = false;
}
