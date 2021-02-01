#include <thread>
#include <assert.h>
#include "thread.h"
#include "process.h"
#include "vm.h"
#include "mm.h"
#include "loop.h"

void
thread_create(thread_t *thread, process_t *process)
{
	thread->process = process;
	vm_create_vcpu(&process->vm, &thread->vcpu);
}

void
thread_init(thread_t *thread, vcpu_sysregs_t *sysregs, mm_gvirt_t rip, mm_gvirt_t rsp)
{
	vcpu_init_sysregs(&thread->vcpu, sysregs);

	vcpu_regs_t regs[] = {
		VCPU_REGS_ENTRY_SET(RIP, rip),
		VCPU_REGS_ENTRY_SET(RSP, rsp),
		VCPU_REGS_ENTRY_SET(RDX, 0),
	};
	vcpu_set_regs(&thread->vcpu, regs, countof(regs));
}

static void
start_thread(thread_t *thread)
{
	do {
		vcpu_run(&thread->vcpu);
		handle_vmexit(thread);
	} while (thread->vcpu.in_operation);
}

void
thread_run(thread_t *thread)
{
	std::thread t(start_thread, thread);

	for (int i = 0; i < 1000000000; i++)
		(void)0;
	vcpu_stop(&thread->vcpu);
	t.join();
}