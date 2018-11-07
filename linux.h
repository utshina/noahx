#pragma once

#define SYSCALLS \
	SYSCALL(brk, 12), \
	SYSCALL(getuid, 102), \
	SYSCALL(getgid, 104), \
	SYSCALL(geteuid, 107), \
	SYSCALL(getegid, 108),

#define SYSCALL(name, number) LINUX_SYS_##name = number
enum {
	SYSCALLS
};

extern char *syscall_name[];