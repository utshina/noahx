#include <unistd.h>
#include "process.h"
#include "panic.h"

ssize_t
sys_write(thread_t *thread, int fd, mm_gvirt_t gbuf, size_t count)
{
	void *buf = malloc(count);
	if (buf == NULL)
		panic("out of memory");

	mm_t *mm = &thread->process->mm;
	mm_copy_from_user(mm, buf, gbuf, count)
		OR panic("copy error");
	
	return write(fd, buf, count);
}