#pragma once

#include "mm.h"

typedef struct {
	vcpu_t vcpu;
} thread_t;

void
thread_create(thread_t *thread, vm_t *vm);

void
thread_init(thread_t *thread, vcpu_sysregs_t *sysregs, mm_gvirt_t rip, mm_gvirt_t rsp);
