#include <unistd.h>
#include "process.h"
#include "utsversion.h"

#define LEN 65

static struct {
	char sysname[LEN];
	char nodename[LEN];
	char release[LEN];
	char version[LEN];
	char machine[LEN];
	char domainname[LEN];
} utsname = {
	.sysname = "Linux",
	.release = "4.19.noahx.x86_64",
	.machine = "x86_64",
	.domainname = "",
};

int
sys_uname(thread_t *thread, mm_gvirt_t buf)
{
	mm_t *mm = &thread->process->mm;
	gethostname(utsname.nodename, sizeof(utsname.nodename));
	strcpy(utsname.version, utsversion);
	mm_copy_to_user(mm, buf, &utsname, sizeof(utsname));
	return 0;
}