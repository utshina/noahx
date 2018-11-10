#include <stdio.h>
#include "syscall.h"
#include "linux.h"
#include "panic.h"

#define SYSCALL(name, number) [number] = #name,
static char *syscall_name[329] = {
	SYSCALLS
};
#undef SYSCALL

#define SYSCALL(name, number) extern uint64_t sys_##name();
SYSCALLS
#undef SYSCALL

#define SYSCALL(name, number) [number] = (syscall_t) sys_##name,
syscall_t syscalls[329] = {
	SYSCALLS
};
#undef SYSCALL


long
handle_syscall(thread_t *thread, uint64_t sysnum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	printf("%s[%ld](%lx,%lx,%lx,%lx,%lx,%lx)\n", syscall_name[sysnum], sysnum, arg1, arg2, arg3, arg4, arg5, arg6);

	syscall_t syscall = syscalls[sysnum];
	if (syscall == NULL)
		panic("unimplemented syscall");
	return syscall(thread, arg1, arg2, arg3, arg4, arg5, arg6);

#if 0
	switch (sysnum)
	{
	case LINUX_SYS_brk:
		return brk(thread, arg1);

	case LINUX_SYS_uname:
		return uname(thread, arg1);

	case LINUX_SYS_readlink: {
		char path[PATH_MAX];
		mm_copy_from_user(mm, path, arg1, PATH_MAX);
		printf("readlink(%s, %lx, %lx)\n", path, arg3, arg4);
	}

	case LINUX_SYS_getuid:
	case LINUX_SYS_getgid:
	case LINUX_SYS_geteuid:
	case LINUX_SYS_getegid:
		return 1000;

	case LINUX_SYS_arch_prctl:
		return arch_prctl(thread, arg1, arg2);

	default:
		panic("unimplemented syscall");
		return -1;
	}
#endif
}
