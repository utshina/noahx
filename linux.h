#pragma once

#define PATH_MAX 4096

#define SYSCALLS \
	SYSCALL(brk, 12) \
	SYSCALL(uname, 63) \
	SYSCALL(readlink, 89) \
	SYSCALL(getuid, 102) \
	SYSCALL(getgid, 104) \
	SYSCALL(geteuid, 107) \
	SYSCALL(getegid, 108) \
	SYSCALL(arch_prctl, 158)

#define SYSCALL(name, number) LINUX_SYS_##name = number,
enum {
	SYSCALLS
};
#undef SYSCALL

enum {
	LINUX_EACCES = 13,
};
