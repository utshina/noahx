#include "Process.hpp"
#include "Debug.hpp"

Process::Process() : m_mm(m_vm)
{
//	m_mm = std::make_unique<Mm>(m_vm);
	LOG("create\n");
}

Process::~Process()
{
	LOG("delete\n");
}