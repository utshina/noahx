#include "process.h"
#include "panic.h"

int
sys_arch_prctl(thread_t *thread, int code, uint64_t arg2)
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
