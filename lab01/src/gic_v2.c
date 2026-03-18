#include <arm-gic.h>
#include "io.h"
#include <asm/irq.h>

struct gic_chip_data {
    unsigned long raw_dist_base; // 分发器基地址
    unsigned long raw_cpu_base;  // CPU interface 基地址
    struct irq_domain *domain; // 中断域
    struct irq_chip *chip; // 中断控制器
    unsigned int gic_irqs; // GIC 支持的中断数量
};

#define gic_dist_base(gic) ((gic)->raw_dist_base)
#define gic_cpu_base(gic) ((gic)->raw_cpu_base)

#define ARM_GIC_MAX_NR 1 // 目前只支持一个 GIC
static struct gic_chip_data gic_data[ARM_GIC_MAX_NR];

/* IRQs start ID */                                                               
#define HW_IRQ_START 16

static unsigned long gic_get_dist_base(void)
{
       struct gic_chip_data *gic = &gic_data[0];

       return gic_dist_base(gic);
}

static unsigned long gic_get_cpu_base(void)
{
       struct gic_chip_data *gic = &gic_data[0];

       return gic_cpu_base(gic);
}

static void gic_set_irq(int irq, unsigned int offset)
{
    // 根据芯片手册，GICD_ISENABLER 寄存器每 32 位对应一个中断的使能位，因此需要计算出对应的寄存器地址和位位置
    unsigned int mask = 1 << (irq % 32);

    writel(mask, gic_get_dist_base() + offset + (irq / 32) * 4);
}

void gicv2_unmask_irq(int irq)
{
       gic_set_irq(irq, GIC_DIST_ENABLE_SET);
}

void gicv2_mask_irq(int irq)
{
	gic_set_irq(irq, GIC_DIST_ENABLE_CLEAR);
}

void gicv2_eoi_irq(int irq)
{
       writel(irq, gic_get_cpu_base() + GIC_CPU_EOI);
}

static unsigned int gic_get_cpumask(struct gic_chip_data *gic)
{
       unsigned long base = gic_dist_base(gic);
       unsigned int mask, i;

       for (i = mask = 0; i < 32; i += 4) {
               mask = readl(base + GIC_DIST_TARGET + i);
               mask |= mask >> 16;
               mask |= mask >> 8;
               if (mask)
                       break;
       }

       return mask;
}

static void gic_dist_init(struct gic_chip_data *gic)
{
       unsigned long base = gic_dist_base(gic);
       unsigned int cpumask;
       unsigned int gic_irqs = gic->gic_irqs;
       int i;

       /* 关闭中断*/
       writel(GICD_DISABLE, base + GIC_DIST_CTRL);

       /* 设置中断路由：GIC_DIST_TARGET
        *
        * 前32个中断(SGI/PPI)怎么路由是GIC芯片固定的，因此先读GIC_DIST_TARGET前面的值
        * 然后全部填充到 SPI的中断号 */
       cpumask = gic_get_cpumask(gic);
       cpumask |= cpumask << 8;
       cpumask |= cpumask << 16;

       for (i = 32; i < gic_irqs; i += 4)
               writel(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

       /* 设置低电平触发 */
       for (i = 32; i < gic_irqs; i += 16)
               writel(GICD_INT_ACTLOW_LVLTRIG, base + GIC_DIST_CONFIG + i / 4);

       /* Deactivate and disable all 中断（SGI， PPI， SPI）.
        *
        * 当注册中断的时候才 enable某个一个SPI中断，例如调用gic_unmask_irq()*/
       for (i = 0; i < gic_irqs; i += 32) {
               writel(GICD_INT_EN_CLR_X32, base +
                               GIC_DIST_ACTIVE_CLEAR + i / 8);
               writel(GICD_INT_EN_CLR_X32, base +
                               GIC_DIST_ENABLE_CLEAR + i / 8);
       }

       /*打开SGI中断（0～15），可能SMP会用到，所以初始化会默认打开*/
       writel(GICD_INT_EN_SET_SGI, base + GIC_DIST_ENABLE_SET);

       /* 打开中断：Enable group0 and group1 interrupt forwarding.*/
       writel(GICD_ENABLE, base + GIC_DIST_CTRL);
}

void gic_cpu_init(struct gic_chip_data *gic)
{
       int i;
       unsigned long base = gic_cpu_base(gic);
       unsigned long dist_base = gic_dist_base(gic);

       /*
        * 设置 PPI 和 SGI 的优先级，GICD_PRI 寄存器每 8 位对应一个中断的优先级，因此每次写入 4 个中断的优先级
         * 这里设置为 0xa0，表示中等优先级，数值越小优先级越高
         * 0xa0a0a0a0 表示一次性写入该寄存器 4 个中断的优先级，分别是 0xa0, 0xa0, 0xa0, 0xa0
        */
       for (i = 0; i < 32; i += 4)
               writel(0xa0a0a0a0,
                       dist_base + GIC_DIST_PRI + i * 4 / 4);

       writel(GICC_INT_PRI_THRESHOLD, base + GIC_CPU_PRIMASK);

       writel(GICC_ENABLE, base + GIC_CPU_CTRL);
}

/* 中断处理函数 */
void gic_handle_irq(void)
{
       struct gic_chip_data *gic = &gic_data[0];
       unsigned long base = gic_cpu_base(gic);
       unsigned int irqstat, irqnr;

       /* 中断处理 */
       do {
            irqstat = readl(base + GIC_CPU_INTACK);
            irqnr = irqstat & GICC_IAR_INT_ID_MASK;

            /* 读取 GICC_IAR 寄存器，获取中断号
            *  并根据中断号处理对应的中断，例如如果是 GENERIC_TIMER_IRQ timer中断 就调用 handle_timer_irq() 函数处理定时器中断
            *  最后调用 gicv2_eoi_irq() 函数向 GICv2 发送 End of Interrupt (EOI) 信号，告诉 GICv2 中断处理完成，可以继续处理下一个中断
            */
            if (irqnr == GENERIC_TIMER_IRQ)
                    handle_timer_irq();

            gicv2_eoi_irq(irqnr);

       } while (0);

}

int gic_init(int chip, unsigned long dist_base, unsigned long cpu_base)
{
    struct gic_chip_data *gic;
    int gic_irqs;
    int virq_base;

    gic = &gic_data[chip];

    /* 1. 初始化基地址 */
    gic->raw_dist_base = dist_base;
    gic->raw_cpu_base = cpu_base;

    /* 2. 计算 GIC 支持的中断数量 */
    gic_irqs = readl(gic_dist_base(gic) + GIC_DIST_CTRL) & 0x1f; //0x1f 是 GICD_TYPER 寄存器的前 5 位，表示支持的中断数量
    gic_irqs = (gic_irqs + 1) * 32; // 每个 GICD_TYPER 寄存器表示支持 32 个中断，所以需要加 1 后乘以 32 才能得到实际支持的中断数量
    if (gic_irqs > 1020) // GICv2 最大支持 1020 个中断
        gic_irqs = 1020;
    gic->gic_irqs = gic_irqs;

    printk("%s: cpu_base:0x%x, dist_base:0x%x, gic_irqs:%d\n",
                       __func__, cpu_base, dist_base, gic->gic_irqs);

    /* 3. 初始化 GIC 分发器 */
    gic_dist_init(gic);
    /* 4. 初始化 GIC CPU interface */
    gic_cpu_init(gic);

    return 0;
}