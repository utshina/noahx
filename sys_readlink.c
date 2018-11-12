#include <unistd.h>
#include "mm.h"
#include "linux.h"

ssize_t
sys_readlink(mm_gvirt_t pathname, mm_gvirt_t buf, size_t size)
{
    return -LINUX_EACCES;
}