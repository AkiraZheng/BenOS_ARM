#include "asm/pl_uart.h"
#include "io.h"
#include "virt.h"
#include "ptregs.h"
#include "error.h"

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

static int check_uart_mmio_range(unsigned long addr)
{
	if (addr >= U_BASE && addr <= U_IMSC_REG)
		return 1;
	else
		return 0;
}

int check_emul_mmio_range(unsigned long addr)
{
	return check_uart_mmio_range(addr);
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
            printk("%s unknown emu_mmio read size\n", __func__);
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
            printk("%s unknown emu write size\n", __func__);
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
	printk("%s : esr 0x%lx iss 0x%x addr 0x%lx width %d write %d reg %d reg_width %d sign_ext %d\n",
		       	__func__, esr, iss, emu_mmio.addr, emu_mmio.width, emu_mmio.write, emu_mmio.reg,
			emu_mmio.reg_width, emu_mmio.sign_ext);
#endif

	emu_mmio_regs(&emu_mmio);

	/* 异常返回的时候，返回触发异常的下一条指令*/
	pc_step = 2 + (2 * il);
	regs->pc += pc_step; 

	return 0;
}
