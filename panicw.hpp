#pragma once

#include "panic.h"
#include <windows.h>

static const HRESULT SUCCESS = 0x80000000;
#undef OR
#define OR & SUCCESS ^ SUCCESS ?(void)0:

void
panicw(HRESULT result, const char *msg);
