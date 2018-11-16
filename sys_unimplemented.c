#include "process.h"
#include "panic.h"

long
sys_unimplemented(thread_t *thread, uint64_t *args)
{
	panic("unimplemented syscall");
}
