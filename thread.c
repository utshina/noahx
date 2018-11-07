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
		VCPU_REGS_ENTRY_SET(RBP, rsp),
	};
	vcpu_set_regs(&thread->vcpu, regs, countof(regs));
}

void
thread_run(thread_t *thread)
{
	do {
		vcpu_run(&thread->vcpu);
		handle_vmexit(thread);
	} while (thread->vcpu.in_operation);
}