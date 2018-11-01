#include "vmm.h"
#include "syscall.h"
#include "linux.h"

uint64_t
handle_syscall(vm_t *vm)
{
	enum { RAX = 0, RDI, RSI, RDX, R10, R8, R9 };
	vmm_regs_t regs[] = {
		REGS_ENTRY_GET(RAX),
		REGS_ENTRY_GET(RDI),
		REGS_ENTRY_GET(RSI),
		REGS_ENTRY_GET(RDX),
		REGS_ENTRY_GET(R10),
		REGS_ENTRY_GET(R8),
		REGS_ENTRY_GET(R9),
	};
	vmm_get_regs(vm, regs, countof(regs));

	uint64_t rax = regs[RAX].value.Reg64;
	switch (rax) {
	case LINUX_sys_geteuid:
		printf("geteuid\n");
		return 501;

	default:
		printf("unimplemented syscall: %lx\n", rax);
	}
}
