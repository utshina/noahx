#include <unistd.h>
#include "process.h"

long
sys_exit_group(thread_t *thread, uint64_t *args)
{
	_exit(args[0]);
}