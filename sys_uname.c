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
	"Linux",
	"",
	"4.19.noahx.x86_64",
	"",
	"x86_64",
	"",
};

int
sys_uname(thread_t *thread, uint64_t *args)
{
	mm_gvirt_t buf = args[0];
	mm_t *mm = &thread->process->mm;
	gethostname(utsname.nodename, sizeof(utsname.nodename));
	strcpy(utsname.version, utsversion);
	mm_copy_to_user(mm, buf, &utsname, sizeof(utsname));
	return 0;
}
