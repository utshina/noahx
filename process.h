#pragma once

#include "thread.h"
#include "mm.h"
#include "vm.h"

typedef struct process {
	int thread_count, thread_count_max;
	thread_t *thread;
	mm_t mm;
	vm_t vm;
} process_t;

void
process_create(process_t *process);

void
process_load(process_t *process, int argc, char *argv[]);

void
process_run(process_t *process);
