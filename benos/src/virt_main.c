#include "io.h"
#include "virt.h"

void virt_main()
{
	unsigned long el;

	el = read_sysreg(CurrentEL);
	printk("prv el = %d\n", el >> 2);
	printk("jumping to VM...\n");

	jump_to_vm();

	unsigned long cur;
	cur = read_sysreg(CurrentEL) >> 2;

	printk("current el = %d\n", cur);

}
