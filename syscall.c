#include <stdio.h>
#include "syscall.h"
#include "linux.h"
#include "panic.h"

#define SYSCALL(name, number) [number] = #name,
static char *syscall_name[] = {
	[0 ... SYSCALL_MAX - 1] = "unimplemented",
	SYSCALLS
};
#undef SYSCALL

#define SYSCALL(name, number) extern uint64_t sys_##name();
SYSCALLS
#undef SYSCALL

static long
unimplemented(thread_t *thread, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	panic("unimplemented syscall");
}

#define SYSCALL(name, number) [number] = (syscall_t) sys_##name,
syscall_t syscalls[] = {
	[0 ... SYSCALL_MAX - 1] = unimplemented,
	SYSCALLS
};
#undef SYSCALL

long
handle_syscall(thread_t *thread, uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	printf("%s[%ld](%lx,%lx,%lx,%lx,%lx,%lx)\n", syscall_name[number], number, arg1, arg2, arg3, arg4, arg5, arg6);

	if (number >= SYSCALL_MAX)
		unimplemented(thread, arg1, arg2, arg3, arg4, arg5, arg6);
	syscall_t syscall = syscalls[number];
	return syscall(thread, arg1, arg2, arg3, arg4, arg5, arg6);
}
