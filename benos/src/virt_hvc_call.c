#include "ptregs.h"
#include "io.h"

void vm_hvc_handler(struct pt_regs *regs)
{
	unsigned int el = read_sysreg(CurrentEL) >> 2;

	printk("%s jump into hypervisor(el=%d), hvccall num: %d\n", __func__, el, regs->regs[0]);
}
