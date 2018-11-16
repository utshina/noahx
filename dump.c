#include <stdio.h>
#include "dump.h"

void
dump_guest_stack(mm_t *mm, mm_gvirt_t rsp)
{
	int skip = rsp % 16;
	rsp = rounddown(rsp, 16);
	int used_stack_size = mm->stack_top - rsp;
	char *p = (char *)mm_stack_gvirt_to_hvirt(mm, rsp);
	for (int i = 0; i < used_stack_size; i++) {
		if ((uint64_t)&p[i] % 16 == 0)
			fprintf(stderr, "\n%lx: ", rsp + i);
		if (skip == 0)
			fprintf(stderr, "%02x ", p[i] & 0xff);
		else {
			fprintf(stderr, "   ");
			skip--;
		}

	}
	fprintf(stderr, "\n");
}
