#pragma once

#include "process.h"

int
handle_syscall(process_t *process, uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);
