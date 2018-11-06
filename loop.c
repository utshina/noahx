#include <stdio.h>
#include "process.h"
#include "mm.h"
#include "syscall.h"
#include "dump.h"

static char *
get_exit_reason_str(int reason)
{
	switch (reason) {
		case WHvRunVpExitReasonNone:
		return "none";
		case WHvRunVpExitReasonMemoryAccess:
		return "memory";
		case WHvRunVpExitReasonUnrecoverableException:
		return "exception";
		default:
		return "undefined";
	}
}

void
handle_vmexit(process_t *process)
{
	mm_t *mm = &process->mm;
	vm_t *vm = &process->vm;
	vcpu_t *vcpu = &process->thread->vcpu;
	int reason = vm->exit_context.ExitReason;
	if (reason == WHvRunVpExitReasonUnrecoverableException) {
		uint16_t *inst = mm_gvirt_to_hvirt(mm, vm->exit_context.VpContext.Rip);
		if (*inst == 0x050f) { // syscall
			vcpu_regvalue_t rip, rax, rdi, rsi, rdx, r10, r8, r9;
			vcpu_regs_t regs1[] = {
				VCPU_REGS_ENTRY_GET(RAX, &rax),
				VCPU_REGS_ENTRY_GET(RDI, &rdi),
				VCPU_REGS_ENTRY_GET(RSI, &rsi),
				VCPU_REGS_ENTRY_GET(RDX, &rdx),
				VCPU_REGS_ENTRY_GET(R10, &r10),
				VCPU_REGS_ENTRY_GET(R8, &r8),
				VCPU_REGS_ENTRY_GET(R9, &r9),
			};
			vcpu_get_regs(vcpu, regs1, countof(regs1));

			rax = handle_syscall(vm, rax, rdi, rsi, rdx, r10, r8, r9);

			rip = vm->exit_context.VpContext.Rip + 2;
			vcpu_regs_t regs2[] = {
				VCPU_REGS_ENTRY_SET(RIP, rip),
				VCPU_REGS_ENTRY_SET(RAX, rax),
			};
			vcpu_set_regs(vcpu, regs2, countof(regs2));
			vm->in_operation = true;
			return;
		}
	}

	vm->in_operation = false;
	printf("exit reason: %s (%x)\n", get_exit_reason_str(reason), reason);
	printf("state: %x\n", vm->exit_context.VpContext.ExecutionState.AsUINT16);
	printf("cs: %x\n", vm->exit_context.VpContext.Cs.Selector);
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
#if 0
		vcpu_regvalue_t rsp;
		vcpu_regs_t regs[] = {
			VCPU_REGS_ENTRY_GET(RSP, &rsp),
		};
		vcpu_get_regs(vcpu, regs, countof(regs));
		dump_guest_stack(mm, rsp);
#endif
		break;

	case WHvRunVpExitReasonUnrecoverableException:
		printf("info: %x\n", vm->exit_context.VpException.ExceptionInfo.AsUNIT32);
		printf("type: %d\n", vm->exit_context.VpException.ExceptionType);
		printf("error code: %d\n", vm->exit_context.VpException.ErrorCode);
		printf("inst count: %d\n", vm->exit_context.VpException.InstructionByteCount);
		printf("parameter: %llx\n", vm->exit_context.VpException.ExceptionParameter);

		char *p = mm_gvirt_to_hvirt(mm, vm->exit_context.VpContext.Rip);
		if (p != NULL)
			printf("inst: %02x %02x\n", *p, *(p+1));



	default:
		;
	}
	return;
}
