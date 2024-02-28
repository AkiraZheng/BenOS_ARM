#include "sysregs.h"
#include "irq.h"
#include <asm/base.h>

extern char gos_vectors[];

void gos_main()
{
	unsigned long val;

	write_sysreg(gos_vectors, vbar_el1);

	gos_paging_init();

	val = read_sysreg(cntv_tval_el0);
	printk("%s == cntv_tval_el0 0x%lx\n", __func__, val);

	gos_gic_init(0, GIC_V2_DISTRIBUTOR_BASE, GIC_V2_CPU_INTERFACE_BASE);

	gos_vtimer_init();

	raw_local_irq_enable();
}
