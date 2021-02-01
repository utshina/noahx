#pragma once

#define OR ?(void)0:

void
_panic(const char *msg);

#define panic(msg) _panic(__FILE__ ": " msg)
