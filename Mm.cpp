#include "Mm.hpp"
#include "Debug.hpp"

Mm::Mm(Vm& vm) : m_vm(vm)
{
	LOG("create\n");
}

Mm::~Mm()
{
	LOG("delete\n");
}