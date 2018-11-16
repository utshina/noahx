#include <unistd.h>
#include "process.h"
#include "linux.h"

long
sys_readlink(thread_t *thread, uint64_t *args)
{
	return -LINUX_EACCES;
}