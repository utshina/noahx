#include <stdio.h>
#include "syscall.h"
#include "linux.h"
#include "panic.h"

#define SYSCALL(number, name) #name,
static const char *syscall_name[] = {
	SYSCALLS
};
#undef SYSCALL

#define SYSCALL(number, name) extern long sys_##name(thread_t *thread, uint64_t *args);
SYSCALLS
#undef SYSCALL

#define SYSCALL(number, name) (syscall_t) sys_##name,
syscall_t syscalls[] = {
	SYSCALLS
};
#undef SYSCALL

long
handle_syscall(thread_t *thread, uint64_t number, uint64_t *args)
{
	printf("%s[%ld](%lx,%lx,%lx,%lx,%lx,%lx)\n", syscall_name[number], number, args[0], args[1], args[2], args[3], args[4], args[5]);

	if (number >= countof(syscalls))
		sys_unimplemented(thread, args);
	syscall_t syscall = syscalls[number];
	return syscall(thread, args);
}
