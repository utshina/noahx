#pragma once

#include <stdnoreturn.h>

#define OR ?true:

noreturn void
_panic(char *msg);

#define panic(msg) _panic(__FILE__ ": " msg)