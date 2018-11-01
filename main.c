#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vmm.h"
#include "load.h"
#include "loop.h"

int
main(int argc, char *argv[])
{
	vm_t vm;

	if (argc < 2) {
		printf("Usage: %s [ELF file]\n", argv[0]);
		exit(1);
	}
	printf("Starting Noahx ...\n");

	vmm_init();
	vmm_create_vm(&vm);
	vmm_setup_vm(&vm);
	load_elf(&vm, argv[1]);
	vmm_create_vcpu(&vm);

//	range_print((void *)&vm.mmap_range_root->range);
	dump_guest_stack(&vm, vm.regs.rsp);
	do {
		vmm_run(&vm);
		handle_vmexit(&vm);
	} while (vm.in_operation);
}
