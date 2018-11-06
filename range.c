#include <stdio.h>
#include <stddef.h>
#include "range.h"

static inline int
range_search_internal(range_root_t root, uint64_t start, uint64_t end, range_t **parent)
{
	int n = 0;
	range_t *r = *root;

	while (r) {
		*parent = r;
		if (r->end <= start)
			n = 1, r = r->right;
		else if (end <= r->start)
			n = -1, r = r->left;
		else
			return 0;
	}
	return n;
}

range_t *
range_search(range_root_t root, uint64_t start, uint64_t end)
{
	range_t *parent = NULL;
	int n = range_search_internal(root, start, end, &parent);
	return (n == 0 ? parent : NULL);
}

range_t *
range_insert(range_root_t root, range_t *range)
{
	range_t *parent = NULL;
	int n = range_search_internal(root, range->start, range->end, &parent);
	if (parent != NULL) {
		if (n < 0)
			parent->left = range;
		else if (n > 0)
			parent->right = range;
		else
			return parent;
	} else
		*root = range;
	return NULL;
}

static int depth = 0;
void
do_range_print(range_t *range)
{
	if (range->left) {
		depth++; do_range_print(range->left); depth--;
	}
	printf("%*c%08lx-%08lx\n", 4 * depth, ' ', range->start, range->end);
	if (range->right) {
		depth++; do_range_print(range->right); depth--;
	}
}

void
range_print(range_root_t root)
{
	depth = 0;
	do_range_print(*root);
}


#ifdef TEST

#include <stdio.h>
#include "range.h"

range_t *root = NULL;

int main()
{
	range_t stack = { .left = NULL, .right = NULL, .start = 0xb000, .end = 0xc000 };
	range_t code = { .left = NULL, .right = NULL,  .start = 0x4000, .end = 0x4100 };
	range_t data = { .left = NULL, .right = NULL,  .start = 0x4100, .end = 0x4400 };

	range_insert(&root, &stack);
	range_insert(&root, &code);
	range_insert(&root, &data);
	range_print(root);
}

#endif
