#include "sysregs.h"

extern char gos_vectors[];

void gos_main()
{
	unsigned long val;

	write_sysreg(gos_vectors, vbar_el1);

	gos_paging_init();

	val = read_sysreg(cntv_tval_el0);
	printk("%s == cntv_tval_el0 0x%lx\n", __func__, val);
}
