#pragma once

#include <stdint.h>

typedef struct range {
	struct range *left, *right;
	uint64_t start, end;
} range_t;

typedef range_t **range_root_t;

static inline void
range_init(range_t *range, uint64_t start, uint64_t end)
{
	range->left = NULL;
	range->right = NULL;
	range->start = start;
	range->end = end;
}

range_t *
range_search(range_root_t root, uint64_t start, uint64_t end);

static inline range_t *
range_search_one(range_root_t root, uint64_t value)
{
	return range_search(root, value, value);
}

range_t *
range_insert(range_root_t root, range_t *range);

void
range_print(range_root_t root);
