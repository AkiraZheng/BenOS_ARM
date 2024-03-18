#include "esr.h"
#include "asm/base.h"
#include "io.h"
#include <asm/pgtable.h>
#include "virt.h"

void do_vcpu_exit(struct pt_regs *regs, int reason, unsigned int esr)
{
	unsigned long addr;
	unsigned long ec;
	unsigned long iss;
	unsigned long il;

	addr = read_sysreg(far_el2);

	iss = esr & ESR_ELx_ISS_MASK;
	il = (esr & ESR_ELx_IL) >> ESR_ELx_IL_SHIFT;
	ec = ESR_ELx_EC(esr);

	printk("%s esr 0x%lx ec 0x%x iss 0x%x il 0x%x\n", __func__, esr, ec, iss, il);

	panic();

}

int do_vm_mem_abort(unsigned long fault_ipa, unsigned int esr, struct pt_regs *regs)
{
	unsigned long ec = ESR_ELx_EC(esr);
	unsigned long prot;
	unsigned long far = read_sysreg(far_el2);

	/* hpfar_el2 左移8位等于真实地址*/
	fault_ipa = fault_ipa << 8;
	fault_ipa &= PAGE_MASK;
	//printk("%s fault IPA 0x%lx far 0x%lx ec 0x%x\n", __func__, fault_ipa, far, ec);

	if ((fault_ipa >= PBASE) && fault_ipa < (PBASE + DEVICE_SIZE))
		prot = S2_PAGE_DEVICE;
	else if (ec == ESR_ELx_EC_IABT_LOW)
		prot = S2_PAGE_NORMAL | S2_AP_RO ;
	else
		prot = S2_PAGE_NORMAL | S2_AP_RW;

	/* 对于解析指令，要用far_el2寄存器读出来的fault_address*/
	if (check_emul_mmio_range(far)) 
		return emul_device(regs, far, esr);

	/* 对于构建S2缺页异常的页表，用hpfar_el2寄存器读出来的IPA */
	return stage2_page_fault(fault_ipa, fault_ipa, PAGE_SIZE, prot);
}
