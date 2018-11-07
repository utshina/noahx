#pragma once

#include "vm.h"

typedef struct process process_t;
typedef struct {
	vcpu_t vcpu;
	process_t *process;
} thread_t;

void
thread_create(thread_t *thread, process_t *process);

void
thread_init(thread_t *thread, vcpu_sysregs_t *sysregs, uint64_t rip, uint64_t rsp);

void
thread_run(thread_t *thread);
