#pragma once

#include "Mm.hpp"
#include "Vm.hpp"
#include <memory>

class Process
{
public:

private:
	Vm m_vm;
	Mm m_mm;

public:
	// CREATORS
	Process();
	~Process();
};