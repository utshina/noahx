#include <unistd.h>
#include "linux.h"

void
sys_exit_group(int status)
{
	_exit(status);
}