#pragma once

#include "Vm.hpp"

class Mm
{
private:
	Vm& m_vm;

public:
	// CREATORS
	Mm(Vm& vm);
	~Mm();
};