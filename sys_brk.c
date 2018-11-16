#include "process.h"

int
sys_brk(thread_t *thread, uint64_t *args)
{
	mm_gvirt_t newbrk = args[0];
	mm_t *mm = &thread->process->mm;
	if (newbrk < mm->heap_start)
		return mm->heap_end;
	newbrk = mm_expand_heap(mm, newbrk);
	return newbrk;
}
