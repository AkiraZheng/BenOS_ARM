#include <asm/irq.h>
#include <io.h>
#include <asm/arm_local_reg.h>

void gos_irq_handle(void)
{
	printk("%s ==\n", __func__);
	gos_gic_handle_irq();
}
