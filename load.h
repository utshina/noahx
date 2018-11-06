#pragma once

#include "mm.h"

typedef struct {
	mm_gvirt_t entry;
	mm_gvirt_t stack;
	mm_gvirt_t heap;
} load_info_t;

void
ldr_load(mm_t *mm, int argc, char *argv[], load_info_t *info);
