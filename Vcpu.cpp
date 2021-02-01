#include "Vcpu.hpp"
#include "Vm.hpp"
#include "panicw.hpp"
#include "Debug.hpp"
#include <iostream>

Vcpu::Vcpu(Vm &vm, UINT32 id) : m_vm(vm), m_id(id)
{
	m_running = false;

	WHvCreateVirtualProcessor(m_vm.getHandle(), m_id, 0)
		OR panic("create virtual processor error");
	LOG("VCPU ", m_id, " created\n");
}

Vcpu::~Vcpu()
{
	WHvDeleteVirtualProcessor(m_vm.getHandle(), m_id)
		OR panic("delete virtual processor error");
	LOG("VCPU ", m_id, " deleted\n");
}

void
Vcpu::init_sysregs(SysRegs *sysregs)
{
	enum {
		Cr0, Cr3, Cr4, Efer, Gdtr, Idtr, Tr,
		Cs, Ss, Ds, Es, Fs, Gs, N
	};
	const WHV_REGISTER_NAME regname[N] = {
#define REGNAME(reg) [reg] = WHvX64Register##reg
		REGNAME(Cr0),
		REGNAME(Cr3),
		REGNAME(Cr4),
		REGNAME(Efer),
		REGNAME(Idtr),
		REGNAME(Gdtr),
		REGNAME(Tr),
		REGNAME(Cs),
		REGNAME(Ss),
		REGNAME(Ds),
		REGNAME(Es),
		REGNAME(Fs),
		REGNAME(Gs),
	};
	WHV_REGISTER_VALUE regvalue[N];
	WHvGetVirtualProcessorRegisters(
		m_vm.getHandle(), m_id, regname, N, regvalue)
		OR panic("get virtual processor registers error");

	static const uint64_t CR0_PE = 1ULL << 0;
	static const uint64_t CR0_PG = 1ULL << 31;
	static const uint64_t CR4_PSE = 1ULL << 4;
	static const uint64_t CR4_PAE = 1ULL << 5;
	static const uint64_t CR4_PGE = 1ULL << 7;
	static const uint64_t CR4_OSFXSR = 1ULL << 9;
	static const uint64_t CR4_OSXMMEXCPT = 1ULL << 10;
	static const uint64_t EFER_LME = 1ULL << 8;
	static const uint64_t EFER_LMA = 1ULL << 10;

	regvalue[Cr0].Reg64 |= (CR0_PE | CR0_PG);
	regvalue[Cr3].Reg64 = sysregs->cr3;
	regvalue[Cr4].Reg64 |= (CR4_PSE | CR4_PAE | CR4_PGE | CR4_OSFXSR | CR4_OSXMMEXCPT);
	regvalue[Efer].Reg64 |= (EFER_LME | EFER_LMA);
	regvalue[Gdtr].Table.Base = sysregs->gdt_base;
	regvalue[Gdtr].Table.Limit = sysregs->gdt_limit;
	regvalue[Idtr].Table.Base = sysregs->idt_base;
	regvalue[Idtr].Table.Limit = sysregs->idt_limit;
	regvalue[Tr].Segment = {
		.Base = sysregs->tss_base, .Limit = sysregs->tss_limit,
		.Selector = 0x10, .Attributes = 0x808b,
	};
	regvalue[Cs].Segment = {
		.Base = 0, .Limit = 0xfffff,
		.Selector = 0x08, .Attributes = 0xa0fb,
	};
	regvalue[Ss].Segment = {
		.Base = 0, .Limit = 0xfffff,
		.Selector = 0x10, .Attributes = 0xc0f3,
	};
	regvalue[Ds].Segment = {};
	regvalue[Es].Segment = {};
	regvalue[Fs].Segment = {};
	regvalue[Gs].Segment = {};
	WHvSetVirtualProcessorRegisters(
		m_vm.getHandle(), m_id, regname, N, regvalue)
		OR panic("set virtual processor registers error");
}

void
Vcpu::get_regs(Reg *regs, int count)
{
	WHV_REGISTER_NAME regname[count];
	WHV_REGISTER_VALUE regvalue[count];

	for (int i = 0; i < count; i++)
		regname[i] = (WHV_REGISTER_NAME)regs[i].name;
	WHvGetVirtualProcessorRegisters(
		m_vm.getHandle(), m_id, regname, count, regvalue)
		OR panic("get virtual processor registers error");
	for (int i = 0; i < count; i++)
		*regs[i].ptr = regvalue[i].Reg64;
}

void
Vcpu::set_regs(Reg *regs, int count)
{
	WHV_REGISTER_NAME regname[count];
	WHV_REGISTER_VALUE regvalue[count];

	for (int i = 0; i < count; i++)
		regname[i] = (WHV_REGISTER_NAME)regs[i].name;
	for (int i = 0; i < count; i++)
		regvalue[i].Reg64 = regs[i].value;
	WHvSetVirtualProcessorRegisters(
		m_vm.getHandle(), m_id, regname, count, regvalue)
		OR panic("set virtual processor registers error");
}

void
Vcpu::set_segbase(RegName seg, uint64_t base)
{
	WHV_REGISTER_NAME regname[1];
	WHV_REGISTER_VALUE regvalue[1];

	memset(regvalue, 0, sizeof(regvalue));
	regname[0] = (WHV_REGISTER_NAME)seg;
	regvalue[0].Segment.Base = base;
	WHvSetVirtualProcessorRegisters(
		m_vm.getHandle(), m_id, regname, 1, regvalue)
		OR panic("set virtual processor registers error");
}

void
Vcpu::run()
{
	WHvRunVirtualProcessor(
		m_vm.getHandle(), m_id,
		&exit_context, sizeof(exit_context))
		OR panic("run virtual processor error");
}

void
Vcpu::stop()
{
	WHvCancelRunVirtualProcessor(
		m_vm.getHandle(), m_id, 0)
		OR panic("cannot stop virtual processor");
}
