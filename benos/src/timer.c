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

static int generic_timer_init(void) { asm volatile( "mov x0, #1\n"
		"msr cntp_ctl_el0, x0"
		:
		:
		: "memory");

	return 0;
}

static int generic_timer_reset(unsigned int val)
{
	asm volatile(
		"msr cntp_tval_el0, %x[timer_val]"
		:
		: [timer_val] "r" (val)
		: "memory");

	return 0;
}

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

static void enable_timer_interrupt(void)
{
	writel(CNT_PNS_IRQ, TIMER_CNTRL0);
}

static void hp_generic_timer_init(void)
{
	write_sysreg(1, cnthp_ctl_el2);
}	

static int hp_generic_timer_reset(unsigned int val)
{
	write_sysreg(val, cnthp_tval_el2);
	return 0;
}

static void hp_enable_timer_interrupt(void)
{
	writel(CNT_HP_IRQ, TIMER_CNTRL0);
}

void timer_init(void)
{
	arch_timer_rate = generic_timer_get_freq();
	printk("cntp freq:0x%x\r\n", arch_timer_rate);
	cpu_khz = arch_timer_rate /= HZ;
	printk("cpu_khz %d \n", arch_timer_rate, cpu_khz);

	boot_tick = read_sysreg(cntpct_el0);
	printk(" boot_tick 0x%lx\n", boot_tick);

#ifdef CONFIG_VIRT
	write_sysreg(0, cntvoff_el2);

	/* EL0/1 access cntpct_el0 without trap into EL2*/
	write_sysreg(1<<0, cnthctl_el2);

	/* disable EL1 physical timer*/
	write_sysreg(0, cntp_ctl_el0);

	/* disblae EL2 Hypervisor physical timer*/
	write_sysreg(0, cnthp_ctl_el2);

	hp_generic_timer_init();
	hp_generic_timer_reset(arch_timer_rate);

	gicv2_unmask_irq(HP_TIMER_IRQ);

	/* enable HCR */
	write_sysreg(1<<4, hcr_el2);

	hp_enable_timer_interrupt();
#else
	generic_timer_init();
	generic_timer_reset(arch_timer_rate);

	gicv2_unmask_irq(GENERIC_TIMER_IRQ);

	enable_timer_interrupt();
#endif
}

void handle_timer_irq(void)
{
#ifdef CONFIG_VIRT
	hp_generic_timer_reset(arch_timer_rate);
	//printk("Core0 Hypervisor Timer interrupt received\r\n");
#else
	generic_timer_reset(arch_timer_rate);
	printk("Core0 Timer interrupt received\r\n");
#endif
	jiffies++;
}

static unsigned int stimer_val = 0;
static unsigned int sval = 200000;

void system_timer_init(void)
{
	stimer_val = readl(TIMER_CLO);
	stimer_val += sval;
	writel(stimer_val, TIMER_C1);

	gicv2_unmask_irq(SYSTEM_TIMER1_IRQ);

	/* enable system timer*/
	writel(SYSTEM_TIMER_IRQ_1, ENABLE_IRQS_0);
}

void handle_stimer_irq(void)
{
	stimer_val += sval;
	writel(stimer_val, TIMER_C1);
	writel(TIMER_CS_M1, TIMER_CS);
	printk("Sytem Timer1 interrupt \n");
}
