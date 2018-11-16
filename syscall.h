#pragma once

#include "process.h"
#include "linux.h"

long
handle_syscall(thread_t *thread, uint64_t number, uint64_t *args);

typedef long (*syscall_t)(thread_t *thread, uint64_t *args);
