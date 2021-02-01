#include "Range.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

int
RangeTree::search_internal(uint64_t const start, uint64_t const end, RangeNode **result)
{
	int n = 0;
	RangeNode *node = root;

	while (node) {
		*result = node;
		if (node->end <= start)
			n = 1, node = node->right;
		else if (end < node->start)
			n = -1, node = node->left;
		else
			return 0;
	}
	return n;
}

RangeNode&
RangeTree::insert(uint64_t const start, uint64_t const end)
{
	RangeNode *result = nullptr;
	RangeNode *node = new RangeNode(start, end);

	int n = search_internal(start, end, &result);
	if (result != nullptr) {
		if (n < 0)
			result->left = node;
		else if (n > 0)
			result->right = node;
		else
			{ delete node; node = result; }
	} else
		root = node;
	return *node;
}

RangeNode*
RangeTree::search(uint64_t const start, uint64_t const end)
{
	RangeNode *result = nullptr;
	int n = search_internal(start, end, &result);
	return (n == 0 ? result : nullptr);
}

void
RangeTree::do_print(RangeNode *node, int depth = 0)
{
	assert(node != nullptr);
	if (node->left) {
		depth++; do_print(node->left, depth); depth--;
	}
	std::cout << std::setw(4*depth) << ' ' << std::setw(8);
	std::cout << std::hex << node->start << '-' << node->end << std::endl;
	if (node->right) {
		depth++; do_print(node->right, depth); depth--;
	}
}

void
RangeTree::print()
{
	do_print(root);
}


#ifdef TEST

int main()
{
	RangeTree tree;
	tree.insert(0xb000, 0xc000);
	tree.insert(0x4000, 0x4100);
	tree.insert(0x4100, 0x4400);
	tree.print();
}

#endif
