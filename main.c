#include "Vm.hpp"
#include "Vcpu.hpp"
#include "Process.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "process.h"
#include "load.h"
#include "loop.h"

int
main(int argc, char *argv[])
{
	process_t process;

	{
		Process process;
	}

	if (argc < 2) {
		printf("Usage: %s [ELF file]\n", argv[0]);
		exit(1);
	}
	printf("Starting Noahx ...\n");

	process_create(&process);
	process_load(&process, argc - 1, &argv[1]);
	process_run(&process);
}
