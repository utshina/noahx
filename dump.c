#include <stdio.h>
#include "vmm.h"

void
dump_guest_stack(vm_t *vm, vmm_gvirt_t rsp)
{
	fprintf(stderr, "gvirt: %lx-%lx\n", rsp, vm->stack_top_gphys);
	fprintf(stderr, "hvirt: %p-%p\n", stack_gphys_to_hvirt(vm, rsp), stack_gphys_to_hvirt(vm, vm->stack_top_gphys));
	int used_stack_size = vm->stack_top_gphys - rsp;
	char *p = stack_gphys_to_hvirt(vm, rsp);
	for (int i = 0; i < used_stack_size; i++) {
		if ((uint64_t)&p[i] % 16 == 0)
			fprintf(stderr, "\n%lx: ", stack_hvirt_to_gphys(vm, &p[i]));
		fprintf(stderr, "%02x ", p[i] & 0xff);
	}
	fprintf(stderr, "\n");
}
