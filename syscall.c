#include <stdio.h>
#include "vm.h"
#include "syscall.h"
#include "linux.h"

int
brk(void *addr)
{
	printf("brk: %p\n", addr);
	return -1;
}

int
handle_syscall(vm_t *vm, uint64_t sysnum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	printf("syscall(%ld)(%lx,%lx,%lx)\n", sysnum, arg1, arg2, arg3);
	switch (sysnum) {
		case LINUX_sys_brk:
		return brk((void *)arg1);

	case LINUX_sys_getuid:
	case LINUX_sys_getgid:
	case LINUX_sys_geteuid:
	case LINUX_sys_getegid:
		return 1000;

	default:
		printf("unimplemented syscall: %ld\n", sysnum);
		return -1;
	}
}
