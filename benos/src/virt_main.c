#include "io.h"
#include "virt.h"

void virt_main()
{
	unsigned long el;

	el = read_sysreg(CurrentEL);
	printk("prv el = %d\n", el >> 2);
	printk("entering to VM...\n");

	write_stage2_pg_reg();

	jump_to_vm();

	unsigned long cur;
	cur = read_sysreg(CurrentEL) >> 2;
	printk("current EL %d\n", cur);

	hvc_call(10);

	//printk("back to VM (el=%d): hvc call done\n", read_sysreg(CurrentEL) >> 2);
	while (1);
}
