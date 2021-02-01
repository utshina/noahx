#pragma once

#include <cstdint>

class RangeTree;

class RangeNode
{
	RangeNode *left = nullptr, *right = nullptr;
	uint64_t start, end;
public:
	friend RangeTree;
	RangeNode(uint64_t const start, uint64_t const end) : start(start), end(end) {};
};

class RangeTree
{
private:
	RangeNode *root = nullptr;

	int
	search_internal(uint64_t start, uint64_t end, RangeNode **result);

	void
	do_print(RangeNode *node, int depth);

public:
	// CREATORS
	RangeTree() {}
	~RangeTree() {}

	// ACCESSORS
	RangeNode*
	search(uint64_t const start, uint64_t const end);

	void
	print();

	// MANIPULATORS
	RangeNode&
	insert(uint64_t start, uint64_t end);
};
