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

static unsigned long arch_timer_rate;

static unsigned long generic_timer_get_freq(void)
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

static int generic_vtimer_reset(unsigned long val)
{
	unsigned long current = read_sysreg(cntvct_el0);
	//printk("%s current 0x%lx val 0x%lx\n", __func__, current ,val);
	write_sysreg(current + val, cntv_cval_el0);
	return 0;
}

void gos_vtimer_init(void)
{
	unsigned long val;

	arch_timer_rate = generic_timer_get_freq();
	printk("gos cntp freq:0x%lx\r\n", arch_timer_rate);
	cpu_khz = arch_timer_rate /= HZ;
	printk("gos cpu_khz %d \n", cpu_khz);

	boot_tick = read_sysreg(cntvct_el0);
	printk("gos boot_tick 0x%lx\n", boot_tick);

	generic_vtimer_reset(arch_timer_rate);

	gos_gicv2_unmask_irq(V_TIMER_IRQ);

	generic_vtimer_init();
}

void gos_handle_vtimer_irq(void)
{
	generic_vtimer_reset(arch_timer_rate);
	printk("GuestOS: vTimer interrupt received\r\n");

	jiffies++;
}
