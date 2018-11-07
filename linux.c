#include "linux.h"

#undef SYSCALL
#define SYSCALL(name, number) [number] = #name
char *syscall_name[329] = {
	SYSCALLS
};
