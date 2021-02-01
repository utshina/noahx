#include <stdio.h>
#include "process.h"
#include "mm.h"
#include "syscall.h"
#include "dump.h"

static const char *
get_exit_reason_str(int reason)
{
	switch (reason)
	{
	case WHvRunVpExitReasonNone:
		return "none";
	case WHvRunVpExitReasonMemoryAccess:
		return "memory";
	case WHvRunVpExitReasonUnrecoverableException:
		return "exception";
	case WHvRunVpExitReasonCanceled:
		return "canceled";
	default:
		return "undefined";
	}
}

static void
print_instruction(mm_t *mm)
{
	WHV_VP_EXIT_CONTEXT *context = &mm->vm->exit_context.VpContext;
	uint8_t len = context->InstructionLength;
	fprintf(stderr, "inst(%d): ", len);
	if (len == 0)
		len = 8;
	uint64_t rip = context->Rip;
	uint8_t *p = (uint8_t *)mm_gvirt_to_hvirt(mm, rip);
	for (int i = 0; i < len; i++)
		fprintf(stderr, "%02x ", p[i]);
	fprintf(stderr, "\n");
}

void handle_vmexit(thread_t *thread)
{
	vcpu_t *vcpu = &thread->vcpu;
	mm_t *mm = &thread->process->mm;
	vm_t *vm = vcpu->vm;

	int reason = vm->exit_context.ExitReason;
	if (reason == WHvRunVpExitReasonUnrecoverableException)
	{
		uint16_t *inst = (uint16_t *)mm_gvirt_to_hvirt(mm, vm->exit_context.VpContext.Rip);
		if (*inst == 0x050f)
		{ // syscall
			uint64_t sysnum;
			uint64_t args[6];
			vcpu_regs_t regs1[] = {
				VCPU_REGS_ENTRY_GET(RAX, &sysnum),
				VCPU_REGS_ENTRY_GET(RDI, &args[0]),
				VCPU_REGS_ENTRY_GET(RSI, &args[1]),
				VCPU_REGS_ENTRY_GET(RDX, &args[2]),
				VCPU_REGS_ENTRY_GET(R10, &args[3]),
				VCPU_REGS_ENTRY_GET(R8, &args[4]),
				VCPU_REGS_ENTRY_GET(R9, &args[5]),
			};
			vcpu_get_regs(vcpu, regs1, countof(regs1));

			vcpu_regvalue_t rax = handle_syscall(thread, sysnum, args);

			vcpu_regvalue_t rip = vm->exit_context.VpContext.Rip + 2;
			vcpu_regs_t regs2[] = {
				VCPU_REGS_ENTRY_SET(RIP, rip),
				VCPU_REGS_ENTRY_SET(RAX, rax),
			};
			vcpu_set_regs(vcpu, regs2, countof(regs2));
			vcpu->in_operation = true;
			return;
		}
	}

	vcpu->in_operation = false;
	printf("exit reason: %s (%x)\n", get_exit_reason_str(reason), reason);
	printf("state: %x\n", vm->exit_context.VpContext.ExecutionState.AsUINT16);
	printf("cs:rip : %x:%llx\n", vm->exit_context.VpContext.Cs.Selector, vm->exit_context.VpContext.Rip);

	switch (vm->exit_context.ExitReason)
	{
	case WHvRunVpExitReasonMemoryAccess:
		print_instruction(mm);
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

	case WHvRunVpExitReasonUnrecoverableException: {
		printf("info: %x\n", vm->exit_context.VpException.ExceptionInfo.AsUINT32);
		printf("type: %d\n", vm->exit_context.VpException.ExceptionType);
		printf("error code: %d\n", vm->exit_context.VpException.ErrorCode);
		printf("inst count: %d\n", vm->exit_context.VpException.InstructionByteCount);
		printf("parameter: %llx\n", vm->exit_context.VpException.ExceptionParameter);
		print_instruction(mm);
		break;
	}

	default:
		break;
	}

	vcpu_regvalue_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, rflags;
	vcpu_regs_t regs[] = {
	    VCPU_REGS_ENTRY_GET(RAX, &rax),
	    VCPU_REGS_ENTRY_GET(RCX, &rcx),
	    VCPU_REGS_ENTRY_GET(RDX, &rdx),
	    VCPU_REGS_ENTRY_GET(RBX, &rbx),
	    VCPU_REGS_ENTRY_GET(RSP, &rsp),
	    VCPU_REGS_ENTRY_GET(RBP, &rbp),
	    VCPU_REGS_ENTRY_GET(RSI, &rsi),
	    VCPU_REGS_ENTRY_GET(RDI, &rdi),
	    VCPU_REGS_ENTRY_GET(R8, &r8),
	    VCPU_REGS_ENTRY_GET(R9, &r9),
	    VCPU_REGS_ENTRY_GET(R10, &r10),
	    VCPU_REGS_ENTRY_GET(R11, &r11),
	    VCPU_REGS_ENTRY_GET(R12, &r12),
	    VCPU_REGS_ENTRY_GET(R13, &r13),
	    VCPU_REGS_ENTRY_GET(R14, &r14),
	    VCPU_REGS_ENTRY_GET(R15, &r15),
	    VCPU_REGS_ENTRY_GET(RFLAGS, &rflags),
	};
	vcpu_get_regs(vcpu, regs, countof(regs));
	fprintf(stderr, "rax=%016lx, rbx=%016lx, rcx=%016lx, rdx=%016lx\n", rax, rbx, rcx, rdx);
	fprintf(stderr, "rsi=%016lx, rdi=%016lx, rbp=%016lx, rsp=%016lx\n", rsi, rdi, rbp, rsp);
	fprintf(stderr, " r8=%016lx,  r9=%016lx, r10=%016lx, r11=%016lx\n", r8, r9, r10, r11);
	fprintf(stderr, "r12=%016lx, r13=%016lx, r14=%016lx, r15=%016lx\n", r12, r13, r14, r15);
	dump_guest_stack(mm, rsp);	

	return;
}
