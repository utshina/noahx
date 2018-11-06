#include <stdio.h>
#include "dump.h"

void
dump_guest_stack(mm_t *mm, mm_gvirt_t rsp)
{
	int used_stack_size = mm->stack_top - rsp;
	char *p = mm_stack_gvirt_to_hvirt(mm, rsp);
	for (int i = 0; i < used_stack_size; i++) {
		if ((uint64_t)&p[i] % 16 == 0)
			fprintf(stderr, "\n%lx: ", rsp + i);
		fprintf(stderr, "%02x ", p[i] & 0xff);
	}
	fprintf(stderr, "\n");
}
