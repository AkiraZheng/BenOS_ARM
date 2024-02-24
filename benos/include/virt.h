#ifndef BENOS_VIRT_H
#define BENOS_VIRT_H

struct emu_mmio_access {
	struct pt_regs *regs;
	unsigned long addr;
	int write;
	int sign_ext;
	int width;
	int reg;
	int reg_width;
};

extern unsigned long jump_to_vm(unsigned long gpa_addr);
extern unsigned long hvc_call(int nr, ...);

extern int emul_device(struct pt_regs *regs, unsigned long fault_addr, unsigned int esr);
extern int check_emul_mmio_range(unsigned long addr);

#endif
