#pragma once

#include <stdnoreturn.h>

#define OR ?(void)0:

_Noreturn void
_panic(const char *msg);

#define panic(msg) _panic(__FILE__ ": " msg)