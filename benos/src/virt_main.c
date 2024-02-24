#include "io.h"
#include "virt.h"

#define GOS_VS_MEM 0x80000000

void virt_main()
{
	unsigned long el;

	unsigned long gpa_addr, gva_addr;

	gpa_addr = get_free_page();
	*(unsigned long *)gpa_addr = 0x12345678;

	printk("In hypervisor gpa_addr/hpa_addr 0x%lx  value: 0x%x\n", gpa_addr, *(unsigned long *)gpa_addr);

	el = read_sysreg(CurrentEL);
	printk("prv el = %d\n", el >> 2);

	write_stage2_pg_reg();

	printk("entering to VM...\n");

	gpa_addr = jump_to_vm(gpa_addr);

	while (1);
}

/* gos start */
void gos_start(unsigned long gpa_addr)
{
	unsigned long gva_addr;
	unsigned long cur;
	cur = read_sysreg(CurrentEL) >> 2;
	printk("current EL %d\n", cur);
	printk("In VM gpa_addr 0x%lx\n", gpa_addr);

	hvc_call(10);

	gva_addr = GOS_VS_MEM;

	printk("\n");
	printk("hvc call done, back to VM\n");
	printk("In VM gpa_addr 0x%lx\n", gpa_addr);

	gos_main();
	gos_mapping(gva_addr, gpa_addr);
	printk("gva_ddr 0x%lx value 0x%lx\n", gva_addr, *(unsigned long *)gva_addr);

	while (1);
}

