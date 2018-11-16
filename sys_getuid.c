#include <unistd.h>
#include "process.h"

long sys_getuid(thread_t *thread, uint64_t *args) { return getuid(); }
long sys_getgid(thread_t *thread, uint64_t *args) { return getgid(); }
long sys_geteuid(thread_t *thread, uint64_t *args) { return geteuid(); }
long sys_getegid(thread_t *thread, uint64_t *args) { return getegid(); }
