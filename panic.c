#include <stdio.h>
#include <stdlib.h>
#include "panic.h"

noreturn void
_panic(char *msg)
{
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

