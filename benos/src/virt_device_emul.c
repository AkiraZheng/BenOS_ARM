#include "asm/pl_uart.h"
#include "io.h"
#include "virt.h"
#include "ptregs.h"
#include "error.h"
#include <arm-gic.h>

#define ESR_IL_OFFSET              (25)
#define ESR_IL_MASK                (1 << 0)

#define ESR_ISS_MASK               (0x1FFFFFF)

#define ESR_ISS_DA_WnR_BIT         (1UL << 6)
#define ESR_ISS_DA_FnV_BIT         (1UL << 10)

#define ESR_ISS_DA_SRT_SHIFT         (16)
#define ESR_ISS_DA_SRT_MASK        (0x1f)

#define ESR_ISS_DA_SF_BIT          (1UL << 15)

#define ESR_ISS_DA_SSE_BIT         (1UL << 21)

#define ESR_ISS_DA_SAS_SHIFT         (22)
#define ESR_ISS_DA_SAS_MASK          (0x3)

#define ESR_ISS_DA_ISV_BIT         (1UL << 24)

int check_uart_mmio_range(unsigned long addr)
{
	if (addr >= U_BASE && addr <= U_IMSC_REG)
		return 1;
	else
		return 0;
}

int check_gic_dist_mmio_range(unsigned long addr)
{
	if (addr >= GIC_V2_DISTRIBUTOR_BASE && 
			addr <= (GIC_V2_DISTRIBUTOR_BASE + GIC_V2_DISTRIBUTOR_SIZE))
		return 1;
	else
		return 0;
}

static int check_arm_local_mmio_range(unsigned long addr)
{
	if (addr >= ARM_LOCAL_BASE && 
			addr <= (ARM_LOCAL_BASE + ARM_LOCAL_SIZE))
		return 1;
	else
		return 0;
}

int check_emul_mmio_range(unsigned long addr)
{
	return check_uart_mmio_range(addr) || check_gic_dist_mmio_range(addr)
		|| check_arm_local_mmio_range(addr);
}

static unsigned long emul_readreg(struct emu_mmio_access *emu_mmio, int reg)
{
	unsigned long val;

	if (reg < 0 || reg > 31)
		return 0;

	switch (emu_mmio->reg_width) {
	case 32:
		val = *(unsigned int *)((unsigned long)emu_mmio->regs + reg * 8);
		break;
	case 64:
		val = *(unsigned long *)((unsigned long)emu_mmio->regs + reg * 8);
		break;
	}

	return val;
}	

static void emul_writereg(struct emu_mmio_access *emu_mmio, int reg, unsigned long val)
{
	if (reg < 0 || reg > 31)
		return;

	switch (emu_mmio->reg_width) {
	case 32:
		*(unsigned int *)((unsigned long)emu_mmio->regs + reg * 8) = val;
		break;
	case 64:
		*(unsigned long *)((unsigned long)emu_mmio->regs + reg * 8) = val;
		break;
	}
}

static unsigned long emul_read_addr(struct emu_mmio_access *emu_mmio)
{
    unsigned long val = 0;

    switch (emu_mmio->width) {
        case 8:
            val = emu_mmio->sign_ext ? *((char *)emu_mmio->addr) : *((unsigned char*)emu_mmio->addr);
            break;
        case 16:
            val = emu_mmio->sign_ext ? *((short *)emu_mmio->addr) : *((unsigned short*)emu_mmio->addr);
            break;
        case 32:
            val = emu_mmio->sign_ext ? *((int *)emu_mmio->addr) : *((unsigned int*)emu_mmio->addr);
            break;
        case 64:
            val = *((unsigned long *)emu_mmio->addr);
            break;
        default:
            printk("%s unknown emu_mmio read size, addr 0x%lx\n", __func__, emu_mmio->addr);
    }

    return val;
}

static void emul_write_addr(struct emu_mmio_access* emu, unsigned long val)
{
    switch (emu->width) {
        case 8:
            *((unsigned char *)emu->addr) = val;
            break;
        case 16:
            *((unsigned short *)emu->addr) = val;
            break;
        case 32:
            *((unsigned int *)emu->addr) = val;
            break;
        case 64:
            *((unsigned long *)emu->addr) = val;
            break;
        default:
            printk("%s unknown emu write size, addr 0x%lx\n", __func__, emu->addr);
    }
}

static int emu_mmio_regs(struct emu_mmio_access *emu_mmio)
{
	unsigned long val;

	if (emu_mmio->write) {
		/* 对于写指令，需要从Xt寄存器中 得到 需要写入的值*/
		val = emul_readreg(emu_mmio, emu_mmio->reg);
		/* 然后写入到 硬件寄存器（MMIO)里*/
		emul_write_addr(emu_mmio, val);
	} else {
		/* 对于读指令，先从异常地址（MMIO)中读取出 硬件寄存器的值*/
		val = emul_read_addr(emu_mmio);
		/* 然后写入到 ptregs->Xt 寄存器中 */
		emul_writereg(emu_mmio, emu_mmio->reg, val);
	}

	return 0;
}

static int emu_gicd_reg(struct emu_mmio_access *emu_mmio, unsigned long reg_off,
		unsigned long reg_group_base, int field_width)
{
	unsigned long old_val, new_val, write_data;

	if (!emu_mmio->write)
		return emu_mmio_regs(emu_mmio); 

	old_val = emul_read_addr(emu_mmio);
	write_data = emul_readreg(emu_mmio, emu_mmio->reg);

	new_val = vgic_emul_update_fields(emu_mmio, reg_off, reg_group_base, field_width, write_data, old_val);

	printk("%s reg 0x%lx old_val 0x%x write_data 0x%x new_val 0x%x\n", __func__, reg_off, old_val, write_data, new_val);

	emul_write_addr(emu_mmio, new_val);
}

static int emu_gicd_ctr(struct emu_mmio_access *emu_mmio)
{
	int enable;

	if (!emu_mmio->write)
		return emu_mmio_regs(emu_mmio);

	enable = emul_read_addr(emu_mmio) & GICD_ENABLE;

	vgic_update_enable(enable);

	return 0;
}

static int emu_gicd_mmio_regs(struct emu_mmio_access *emu_mmio)
{
	unsigned long addr = emu_mmio->addr;
	unsigned long reg_off = addr & 0xfff;

	switch (reg_off){
	case GIC_DIST_CTRL:
		emu_gicd_ctr(emu_mmio);
		break;

	case GIC_DIST_CTR:
	case GIC_DIST_IIDR:
	case GIC_DIST_SOFTINT:
		emu_mmio_regs(emu_mmio);
		break;

	case GIC_DIST_IGROUP ... GIC_DIST_IGROUPn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_IGROUP, 1);
		break;

	case GIC_DIST_ENABLE_SET ... GIC_DIST_ENABLE_SETn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_ENABLE_SET, 1);
		break;

	case GIC_DIST_ENABLE_CLEAR ... GIC_DIST_ENABLE_CLEARn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_ENABLE_CLEAR, 1);
		break;

	case GIC_DIST_PENDING_SET ... GIC_DIST_PENDING_SETn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_PENDING_SET, 1);
		break;

	case GIC_DIST_PENDING_CLEAR ... GIC_DIST_PENDING_CLEARn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_PENDING_CLEAR, 1);
		break;

	case GIC_DIST_ACTIVE_SET ... GIC_DIST_ACTIVE_SETn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_ACTIVE_SET, 1);
		break;

	case GIC_DIST_ACTIVE_CLEAR ... GIC_DIST_ACTIVE_CLEARn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_ACTIVE_CLEAR, 1);
		break;

	case GIC_DIST_PRI ... GIC_DIST_PRIn: 
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_PRI, 8);
		break;

	case GIC_DIST_TARGET ... GIC_DIST_TARGETn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_TARGET, 8);
		break;

	case GIC_DIST_CONFIG ... GIC_DIST_CONFIGn: 
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_CONFIG, 2);
		break;

	case GIC_DIST_NSACR ... GIC_DIST_NSACRn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_NSACR, 2);
		break;

	case GIC_DIST_SGI_PENDING_CLEAR ... GIC_DIST_SGI_PENDING_CLEARn:
		emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_SGI_PENDING_CLEAR, 8);
		break;

	case GIC_DIST_SGI_PENDING_SET ... GIC_DIST_SGI_PENDING_SETn:
	       emu_gicd_reg(emu_mmio, reg_off, GIC_DIST_SGI_PENDING_SET, 8);
	       break;

	default:
	       printk("%s error: unknown emu addr 0x%lx\n", __func__, emu_mmio->addr);
	       break;
	}

	return 0;
}

static int emu_mmio_range(struct emu_mmio_access *emu_mmio)
{
	if (check_uart_mmio_range(emu_mmio->addr) || check_arm_local_mmio_range(emu_mmio->addr))
	       return emu_mmio_regs(emu_mmio);
	else if (check_gic_dist_mmio_range(emu_mmio->addr))
		return emu_gicd_mmio_regs(emu_mmio);

	return 0;
}

int emul_device(struct pt_regs *regs, unsigned long fault_addr, unsigned int esr)
{
	struct emu_mmio_access emu_mmio;
	unsigned long il, iss;
	unsigned long pc_step;

	il = (esr >> ESR_IL_OFFSET) & ESR_IL_MASK; 

	iss = esr & ESR_ISS_MASK; 

	if (!(iss & ESR_ISS_DA_ISV_BIT) || (iss & ESR_ISS_DA_FnV_BIT))
		return -EFAULT;

	emu_mmio.regs = regs;
	emu_mmio.addr = fault_addr;

	/*出错地址的位宽*/
	emu_mmio.width = (iss >> ESR_ISS_DA_SAS_SHIFT) & ESR_ISS_DA_SAS_MASK; 
	emu_mmio.width = 8 * (1 << emu_mmio.width);

	/* 异常时的指令是 因为读指令还是写指令导致的？*/
	emu_mmio.write = (iss & ESR_ISS_DA_WnR_BIT) ? 1:0;

	/* 异常时的指令的 目标寄存器Xt/Rt*/
	emu_mmio.reg = (iss >> ESR_ISS_DA_SRT_SHIFT) & ESR_ISS_DA_SRT_MASK;  

	/* 目标寄存器位宽*/
	emu_mmio.reg_width = (iss & ESR_ISS_DA_SF_BIT) ? 64:32; 

	/* 是否需要 符合扩展*/
	emu_mmio.sign_ext = (iss & ESR_ISS_DA_SSE_BIT) ? 1:0; 

#if 0
	if (check_gic_dist_mmio_range(fault_addr))
	printk("%s : esr 0x%lx iss 0x%x addr 0x%lx width %d write %d reg %d reg_width %d sign_ext %d\n",
		       	__func__, esr, iss, emu_mmio.addr, emu_mmio.width, emu_mmio.write, emu_mmio.reg,
			emu_mmio.reg_width, emu_mmio.sign_ext);
#endif

	emu_mmio_range(&emu_mmio);

	/* 异常返回的时候，返回触发异常的下一条指令*/
	pc_step = 2 + (2 * il);
	regs->pc += pc_step; 

	return 0;
}
