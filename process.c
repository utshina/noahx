#include "process.h"
#include "load.h"
#include "thread.h"
#include "loop.h"
#include "dump.h"
#include "panic.h"

void
process_create(process_t *process)
{
	vm_create(&process->vm);
	mm_init(&process->mm, &process->vm);
	mm_setup_kernel(&process->mm);
	mm_setup_stack(&process->mm);

	process->thread_count = 1;
	process->thread_count_max = 2;
	process->thread = calloc(process->thread_count_max, sizeof(thread_t));
	if (process->thread == NULL)
		panic("out of memory");
	thread_create(&process->thread[0], &process->vm);
}

void
process_load(process_t *process, int argc, char *argv[])
{
	load_info_t info;
	ldr_load(&process->mm, argc, argv, &info);
	mm_setup_heap(&process->mm, info.heap);

	vcpu_sysregs_t sysregs;
	mm_get_sysregs(&process->mm, &sysregs);
	thread_init(process->thread, &sysregs, info.entry, info.stack);
	dump_guest_stack(&process->mm, info.stack);
}

void
process_run(process_t *process)
{
	// mm_dump_range_tree(&process->mm);
	do {
		vcpu_run(&process->thread->vcpu);
		handle_vmexit(process);
	} while (process->vm.in_operation);
}
