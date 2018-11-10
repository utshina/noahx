#include "process.h"

int
sys_brk(thread_t *thread, mm_gvirt_t newbrk)
{
	mm_t *mm = &thread->process->mm;
	if (newbrk < mm->heap_start)
		return mm->heap_end;
	mm_expand_heap(mm, newbrk);
	return newbrk;
}
