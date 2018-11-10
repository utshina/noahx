#include <unistd.h>

long sys_getuid() { return getuid(); }
long sys_getgid() { return getgid(); }
long sys_geteuid() { return geteuid(); }
long sys_getegid() { return getegid(); }
