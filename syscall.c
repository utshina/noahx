#include <stdio.h>
#include "syscall.h"
#include "linux.h"

int brk(process_t *process, mm_gvirt_t newbrk)
{
	if (newbrk < process->mm.heap_start)
		return process->mm.heap_end;
	mm_expand_heap(&process->mm, newbrk);
	printf("brk: %lx\n", newbrk);
	return newbrk;
}

int handle_syscall(process_t *process, uint64_t sysnum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	printf("%s(%lx,%lx,%lx,%lx,%lx,%lx)\n", syscall_name[sysnum], arg1, arg2, arg3, arg4, arg5, arg6);
	switch (sysnum)
	{
	case LINUX_SYS_brk:
		return brk(process, arg1);

	case LINUX_SYS_getuid:
	case LINUX_SYS_getgid:
	case LINUX_SYS_geteuid:
	case LINUX_SYS_getegid:
		return 1000;

	default:
		printf("unimplemented syscall: %ld\n", sysnum);
		return -1;
	}
}
