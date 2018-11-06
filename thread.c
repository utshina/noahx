#include <assert.h>
#include "thread.h"
#include "vm.h"
#include "mm.h"

void
thread_create(thread_t *thread, vm_t *vm)
{
	vm_create_vcpu(vm, &thread->vcpu);
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