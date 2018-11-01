#include <stdio.h>
#include "vmm.h"
#include "syscall.h"

void
handle_vmexit(vm_t *vm)
{
	printf("exit reason: %x\n", vm->exit_context.ExitReason);
	printf("state: %x\n", vm->exit_context.VpContext.ExecutionState.AsUINT16);
	printf("cs: %x\n", vm->exit_context.VpContext.Cs.Selector);
	printf("rip: %llx\n", vm->exit_context.VpContext.Rip);

	vm->in_operation = false;
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
		enum { RSP };
		vmm_regs_t regs[] = {
			REGS_ENTRY_GET(RSP),
		};
		vmm_get_regs(vm, regs, countof(regs));
		dump_guest_stack(vm, regs[RSP].value.Reg64);
		break;

	case WHvRunVpExitReasonUnrecoverableException:
		printf("info: %x\n", vm->exit_context.VpException.ExceptionInfo.AsUNIT32);
		printf("type: %d\n", vm->exit_context.VpException.ExceptionType);
		printf("error code: %d\n", vm->exit_context.VpException.ErrorCode);
		printf("inst count: %d\n", vm->exit_context.VpException.InstructionByteCount);
		printf("parameter: %llx\n", vm->exit_context.VpException.ExceptionParameter);

		char *p = vmm_gvirt_to_hvirt(vm, vm->exit_context.VpContext.Rip);
		if (p != NULL)
			printf("inst: %02x %02x\n", *p, *(p+1));

		uint16_t *inst = vmm_gvirt_to_hvirt(vm, vm->exit_context.VpContext.Rip);
		if (*inst == 0x050f) { // syscall
			WHV_REGISTER_VALUE rax, rip;
			rax.Reg64 = handle_syscall(vm);
			rip.Reg64 = vm->exit_context.VpContext.Rip + 2;
			enum { RIP = 0, RAX };
			vmm_regs_t regs[] = {
				REGS_ENTRY_SET(RIP, rip),
				REGS_ENTRY_SET(RAX, rax),
			};
			printf("rip: %llx, %llx\n", rip, regs[0].value.Reg64);
			vmm_set_regs(vm, regs, countof(regs));
			vm->in_operation = true;
		}
		break;

	default:
		;
	}
}
