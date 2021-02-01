#pragma once

#include <iostream>
#include <WinHvPlatform.h>
#include <windows.h>

class Vm;

class Vcpu
{
public:
	enum RegName {
		VCPU_REG_RAX = WHvX64RegisterRax,
		VCPU_REG_RCX = WHvX64RegisterRcx,
		VCPU_REG_RDX = WHvX64RegisterRdx,
		VCPU_REG_RBX = WHvX64RegisterRbx,
		VCPU_REG_RSP = WHvX64RegisterRsp,
		VCPU_REG_RBP = WHvX64RegisterRbp,
		VCPU_REG_RSI = WHvX64RegisterRsi,
		VCPU_REG_RDI = WHvX64RegisterRdi,
		VCPU_REG_R8 = WHvX64RegisterR8,
		VCPU_REG_R9 = WHvX64RegisterR9,
		VCPU_REG_R10 = WHvX64RegisterR10,
		VCPU_REG_R11 = WHvX64RegisterR11,
		VCPU_REG_R12 = WHvX64RegisterR12,
		VCPU_REG_R13 = WHvX64RegisterR13,
		VCPU_REG_R14 = WHvX64RegisterR14,
		VCPU_REG_R15 = WHvX64RegisterR15,
		VCPU_REG_RIP = WHvX64RegisterRip,
		VCPU_REG_RFLAGS = WHvX64RegisterRflags,

		VCPU_SEG_FS = WHvX64RegisterFs,
		VCPU_SEG_GS = WHvX64RegisterGs,
	};

	struct Reg {
		RegName name;
		union {
			uint64_t value;
			uint64_t *ptr;
		};
	};

#define VCPU_REGS_ENTRY_GET(reg, val) { VCPU_REG_##reg, { (vcpu_regvalue_t)val } }
#define VCPU_REGS_ENTRY_SET(reg, val) { VCPU_REG_##reg, { val } }

	struct SysRegs {
		uint64_t cr3;
		uint64_t gdt_base;
		uint16_t gdt_limit;
		uint64_t idt_base;
		uint16_t idt_limit;
		uint64_t tss_base;
		uint16_t tss_limit;
	};

private:
	WHV_RUN_VP_EXIT_CONTEXT exit_context;
	Vm& m_vm;
	UINT32 m_id;
	bool m_running;

public:
	Vcpu(Vm& vm, UINT32 id);
	~Vcpu();

	void
	get_regs(Reg *regs, int count);

	void
	set_regs(Reg *regs, int count);

	void
	init_sysregs(SysRegs *sysregs);

	void
	set_segbase(RegName seg, uint64_t base);

	void
	run();

	void
	stop();
};
