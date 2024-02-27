#include <asm/timer.h>
#include <asm/irq.h>
#include <io.h>
#include <asm/arm_local_reg.h>
#include <timer.h>
#include <type.h>

#define HZ 250

unsigned long cpu_khz;
unsigned long boot_tick;

unsigned long volatile cacheline_aligned jiffies;

static unsigned int arch_timer_rate;

static unsigned int generic_timer_get_freq(void)
{
	unsigned int freq;

	asm volatile(
		"mrs %0, cntfrq_el0"
		: "=r" (freq)
		:
		: "memory");

	return freq;
}

static void generic_vtimer_init(void)
{
	write_sysreg(1, cntv_ctl_el0);
}	

static int generic_vtimer_reset(unsigned int val)
{
	write_sysreg(val, cntv_tval_el0);
	return 0;
}

void gos_vtimer_init(void)
{
	arch_timer_rate = generic_timer_get_freq();
	printk("gos cntp freq:0x%x\r\n", arch_timer_rate);
	cpu_khz = arch_timer_rate /= HZ;
	printk("gos cpu_khz %d \n", arch_timer_rate, cpu_khz);

	boot_tick = read_sysreg(cntpct_el0);
	printk("gos boot_tick 0x%lx\n", boot_tick);

	generic_vtimer_init();
	generic_vtimer_reset(arch_timer_rate);

	//gicv2_unmask_irq(HP_TIMER_IRQ);
}

void gos_handle_vtimer_irq(void)
{
	generic_vtimer_reset(arch_timer_rate);
	printk("GuestOS: vTimer interrupt received\r\n");

	jiffies++;
}
