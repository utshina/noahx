#include <stdio.h>
#include "syscall.h"
#include "linux.h"
#include "panic.h"

#include "uname.h"

int brk(mm_t *mm, mm_gvirt_t newbrk)
{
	if (newbrk < mm->heap_start)
		return mm->heap_end;
	mm_expand_heap(mm, newbrk);
	printf("brk: %lx\n", newbrk);
	return newbrk;
}

int arch_prctl(thread_t *thread, int code, uint64_t arg2)
{
	enum
	{
		ARCH_SET_GS = 0x1001,
		ARCH_SET_FS = 0x1002,
		ARCH_GET_FS = 0x1003,
		ARCH_GET_GS = 0x1004,

		ARCH_GET_CPUID = 0x1011,
		ARCH_SET_CPUID = 0x1012,

		ARCH_MAP_VDSO_X32 = 0x2001,
		ARCH_MAP_VDSO_32 = 0x2002,
		ARCH_MAP_VDSO_64 = 0x2003
	};

	switch (code)
	{
	case ARCH_SET_FS:
		vcpu_set_segbase(&thread->vcpu, VCPU_SEG_FS, arg2);
		return 0;

	default:
		panic("unimplemented arch_prctl\n");
	}
}

int handle_syscall(thread_t *thread, uint64_t sysnum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	printf("%s[%ld](%lx,%lx,%lx,%lx,%lx,%lx)\n", syscall_name[sysnum], sysnum, arg1, arg2, arg3, arg4, arg5, arg6);

	mm_t *mm = &thread->process->mm;
	switch (sysnum)
	{
	case LINUX_SYS_brk:
		return brk(mm, arg1);

	case LINUX_SYS_uname:
		return uname(mm, arg1);

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
}
