#include <arm-gic.h>
#include "io.h"
#include <asm/pgtable.h>
#include <irq.h>
#include "virt.h"

struct vgic {
	unsigned long vctrl_base;
	unsigned long cpu_base; /* */
	unsigned long vcpu_base; /* */
	int nr_lr;
};

static struct vgic vgic;


#define gic_vctrl_base(d) ((d)->vctrl_base)

unsigned long vgic_emul_update_fields(struct emu_mmio_access *emul,
		unsigned long reg_off,
		unsigned long reg_group_base, int field_width,
		unsigned long new_val, unsigned long old_val)
{
	int i;

	int irq = ((reg_off - reg_group_base) * 8) / field_width;
	unsigned long mask = (1UL << field_width) - 1;
	int off = 0;
	unsigned long data = 0;
	unsigned long value = 0;
	struct irq_desc *desc;

	for (i = 0; i < (emul->width / field_width); i++) {
		/* if this IRQ ower by HP, don't modify this field*/
		desc = gic_get_irq_desc(irq + i);
		if (!desc)
			return 0;
		if (desc->irq_state == IRQ_STATE_HP)
			continue;

		off = i * field_width;
		data = (new_val >> off) & mask;

		value |= (data << off); 
	}

	return value;
}

static unsigned long gich_get_elrsr(void)
{
	unsigned long elsr = readl(vgic.vctrl_base + GICH_ELRSR0);

	if (vgic.nr_lr > 32)
		elsr |= readl(vgic.vctrl_base + GICH_ELRSR0 + 4) << 32;

    return elsr;
}

static unsigned int gich_get_apr(void)
{
	return readl(vgic.vctrl_base + GICH_APR);
}

unsigned int vgic_new_lr_value_hw(int hwirq)
{
	unsigned int lr = 0;
	unsigned char prio;

	lr |= hwirq & GICH_LR_VIRTUALID;

	prio = gicv2_get_prio(hwirq);
	lr |= (prio >> 3) << GICH_LR_PRIO_SHIFT;

	lr |= GICH_LR_HW;
	lr |= hwirq << GICH_LR_PHYSID_CPUID_SHIFT;
	lr |= GICH_LR_PENDING_BIT;
//
	printk("%s irq %d prio %d lr 0x%x\n", __func__, hwirq, prio, lr);

	return lr;
}

void gich_set_np_int()
{
	unsigned long val;

	val = readl(vgic.vctrl_base + GICH_HCR);
	writel(val | GICH_HCR_NP, vgic.vctrl_base + GICH_HCR);
}

void gich_write_lr(unsigned int lr, int i)
{
	writel(lr, vgic.vctrl_base + GICH_LR0 + (i * 4));
}

static int vgic_set_lr(int hwirq, unsigned int lr_value)
{
	unsigned long elsr;
	unsigned int is_active;
	int i;
	unsigned int prev_lr;

	elsr = gich_get_elrsr();	
	is_active = gich_get_apr();

	for (i = 0; i < vgic.nr_lr; i++) {
		prev_lr = readl(vgic.vctrl_base + GICH_LR0 + 4 * i);
		if (!(elsr & (1<<i)) &&  (hwirq == (prev_lr & 0x1ff))) {
				if (is_active & (1<<i)) {
					gich_set_np_int();
					printk("%s # irq active and pending.\n");
					return 0;
				}

			printk("!!! same irq_no is pending in lr!!!\n");
			printk("prev_lr 0x%x  lr_value 0x%x hwir 0x%x\n", prev_lr, lr_value, hwirq);
			printk("GICV_AIR 0x%x\n", readl(GIC_V2_VCPU_INTERFACE_BASE + GIC_CPU_INTACK));
			printk("GICV_CTL 0x%x\n", readl(GIC_V2_VCPU_INTERFACE_BASE + GIC_CPU_CTRL));
			return -1;
		}
	}

	for (i = 0; i < vgic.vctrl_base; i++) {
		if (elsr & (1<<i)) {
			gich_write_lr(lr_value, i);
			return 0;
		}
	}

	gich_set_np_int();
	printk("irq pending in memory\n");

	return 0;
}

enum irq_res inject_hw_irq(int hwirq)
{
	unsigned int lr_value;

	lr_value = vgic_new_lr_value_hw(hwirq);

	vgic_set_lr(hwirq, lr_value);

	return FORWARD_TO_VM;
}

static void vgicv2_init_lrs(struct vgic *vgic)
{
	int i;
	unsigned long base = gic_vctrl_base(vgic);

	for (i = 0; i < vgic->nr_lr; i++)
		writel(0, base + GICH_LR0 + (i * 4));
}

enum irq_res vgic_maintenance_handler(int hwirq)
{
	unsigned int misr;

	misr = readl(vgic.vctrl_base + GICH_MISR);

	printk("%s misr 0x%lx 0x%x 0x%x\n",
			__func__, misr, readl(vgic.vctrl_base + GICH_EISR0),
			readl(vgic.vctrl_base + GICH_ELRSR0));

	return HANDLED_BY_HYP;
}

void vgicv2_init(unsigned long vctrl_base)
{
	unsigned long val;

	vgic.vctrl_base = vctrl_base;

	printk("%s base 0x%lx 0x%lx\n", __func__, vctrl_base, vgic.vctrl_base);

	val = readl(vctrl_base + GICH_VTR);
	vgic.nr_lr = (val & 0x3f) + 1;

	printk("%s nr lr: %d\n", __func__, vgic.nr_lr);

	vgicv2_init_lrs(&vgic);

#if 1
	stage2_mapping(GIC_V2_CPU_INTERFACE_BASE, GIC_V2_VCPU_INTERFACE_BASE,
			GIC_V2_CPU_INTERFACE_SIZE, S2_PAGE_DEVICE | S2_AP_RW);
#endif

	/* enable vgic*/
	//val = readl(vctrl_base + GICH_HCR);
	writel(GICH_HCR_EN | 1<<2, vctrl_base + GICH_HCR);

	/* enable Maintenance interrupts for GIC*/
	gicv2_unmask_irq(25);
}
