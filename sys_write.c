#include <unistd.h>
#include "process.h"
#include "panic.h"

long
sys_write(thread_t *thread, uint64_t *args)
{
	int fd = args[0];
	mm_gvirt_t gbuf = args[1];
	size_t count = args[2];

	void *buf = malloc(count);
	if (buf == NULL)
		panic("out of memory");

	mm_t *mm = &thread->process->mm;
	mm_copy_from_user(mm, buf, gbuf, count)
		OR panic("copy error");
	
	return write(fd, buf, count);
}