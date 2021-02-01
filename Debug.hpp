#pragma once

#include <iostream>

static const bool DEBUG = true;

template<typename ...Args>
void log(Args && ...args)
{
	if (DEBUG)
		(std::cerr << ... << args);
}

#define LOG(...) log(__func__, ": ", __VA_ARGS__)
