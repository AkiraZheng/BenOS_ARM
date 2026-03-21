#include <asm/pgtable.h>
#include <asm/pgtable_prot.h>
#include <asm/pgtable_hwdef.h>
#include <sysregs.h>
#include <asm/barrier.h>
#include <string.h>
#include <asm/base.h>

#define NO_BLOCK_MAPPINGS BIT(0)
#define NO_CONT_MAPPINGS BIT(1)

extern char idmap_pg_dir[];

extern char _text_boot[], _etext_boot[];
extern char _text[], _etext[];

static void alloc_init_pte(pmd_t *pmdp, unsigned long addr,
               unsigned long end, unsigned long phys,
               unsigned long prot,
               unsigned long (*alloc_pgtable)(void),
               unsigned long flags)
{
       pmd_t pmd = *pmdp;
       pte_t *ptep;

       if (pmd_none(pmd)) {
               unsigned long pte_phys;

               pte_phys = alloc_pgtable();
               set_pmd(pmdp, __pmd(pte_phys | PMD_TYPE_TABLE));
               pmd = *pmdp;
       }

       ptep = pte_offset_phys(pmdp, addr);
       do {
               set_pte(ptep, pfn_pte(phys >> PAGE_SHIFT, prot)); //最后一级了直接配 PFN 值给 entry
               phys += PAGE_SIZE;
       } while (ptep++, addr += PAGE_SIZE, addr != end);
}

void pmd_set_section(pmd_t *pmdp, unsigned long phys,
               unsigned long prot)
{
       unsigned long sect_prot = PMD_TYPE_SECT | mk_sect_prot(prot);

       pmd_t new_pmd = pfn_pmd(phys >> PMD_SHIFT, sect_prot);

       //往 pmdp entry 中填所映射的物理地址的 PFN 值
       set_pmd(pmdp, new_pmd);
}

static void alloc_init_pmd(pud_t *pudp, unsigned long addr,
               unsigned long end, unsigned long phys,
               unsigned long prot,
               unsigned long (*alloc_pgtable)(void),
               unsigned long flags)
{
       pud_t pud = *pudp;
       pmd_t *pmdp;
       unsigned long next;

       if (pud_none(pud)) {
               unsigned long pmd_phys;

               pmd_phys = alloc_pgtable();
               set_pud(pudp, __pud(pmd_phys | PUD_TYPE_TABLE));
               pud = *pudp;
       }

       pmdp = pmd_offset_phys(pudp, addr);
       do {
               next = pmd_addr_end(addr, end);

               if (((addr | next | phys) & ~SECTION_MASK) == 0 && // 地址对齐检查
                               (flags & NO_BLOCK_MAPPINGS) == 0) // flags 检查出是NO_BLOCK_MAPPINGS块映射
                       pmd_set_section(pmdp, phys, prot);
               else
                       alloc_init_pte(pmdp, addr, next, phys,
                                       prot,  alloc_pgtable, flags);//创建下一级 PTE 映射

               phys += next - addr;
       } while (pmdp++, addr = next, addr != end);
}

/**
 * @brief 创建 PUD 页表映射
 *
 * @param pgdp         [in] PGD 条目的指针，用于获取或创建对应的 PUD 页
 * @param addr         [in] 虚拟起始地址
 * @param end          [in] 虚拟结束地址
 * @param phys         [in] 物理起始地址（恒等映射时通常等于 addr）
 * @param prot         [in] 页表保护属性（如 PAGE_KERNEL_ROX）
 * @param alloc_pgtable [in] 页表分配函数指针，用于分配下一级页表
 * @param flags        [in] 映射标志位
 * @return             void 无返回值
 */
static void alloc_init_pud(pgd_t *pgdp, unsigned long addr,
               unsigned long end, unsigned long phys,
               unsigned long prot,
               unsigned long (*alloc_pgtable)(void),
               unsigned long flags)
{
       pgd_t pgd = *pgdp;
       pud_t *pudp;
       unsigned long next;

       /* 创建pud页 
        * 如果pgd[pgd_entry_idx]存的pud页基地址是空的，说明还没建立该页
        */
       if (pgd_none(pgd)) {
               unsigned long pud_phys;

               pud_phys = alloc_pgtable();

               //set_pgd：将__pgd拼接的64位数据填充到pgd页表项entry中
               //__pgd是通过pud的下一级页基地址和PUD_TYPE_TABLE配置拼接起来的64位值
               set_pgd(pgdp, __pgd(pud_phys | PUD_TYPE_TABLE));
               pgd = *pgdp;
       }

       //得到下一级 pud entry 的 物理地址
       pudp = pud_offset_phys(pgdp, addr);
       do {
               next = pud_addr_end(addr, end);//获得下一个pud页的地址，如果当页能覆盖，那么说明不需要next，next=addr
               alloc_init_pmd(pudp, addr, next, phys,
                               prot, alloc_pgtable, flags);
               phys += next - addr;

       } while (pudp++, addr = next, addr != end);
}

/**
 * @brief 创建 size 大小内存的PGD页表映射
 *
 * @param pgdir     [in] PGD页表目录的基地址
 * @param phys    [in] 要映射的物理起始地址
 * @param virt    [in] 要映射的虚拟起始地址
 * @param size    [in] 要映射的内存大小（字节）
 * @param prot    [in] 页表保护属性（如PAGE_KERNEL_ROX）
 * @param alloc_pgtable [in] 分配页表的函数指针，用于分配下一级页表
 * @param flags   [in] 映射标志位
 * @return        void 无返回值
 */
static void __create_pgd_mapping(pgd_t *pgdir, unsigned long phys, unsigned long virt,
                    unsigned long size, unsigned long prot,
                    unsigned long (*alloc_pgtable)(void), unsigned long flags)
{
    //1. 通过PGD页表的基地址+取virt中的PGD索引得到PGD entry的地址
    pgd_t * pgdp = pgd_offset_raw(pgdir, virt);

    //2. 将地址页对齐（清除低12位偏移），确保从页边界开始映射
    unsigned long addr, end, next;
    phys &= PAGE_MASK;  // 物理地址页对齐，只保留 PFN 的位
    addr = virt & PAGE_MASK;  // 虚拟地址页对齐，只保留 VFN 的位
    end = PAGE_ALIGN(virt + size);

    //3. 逐级往下创建页表：pud->pmd->pte
    do {
        next = pgd_addr_end(addr, end);//获得下一个pgd页的地址，如果当页能覆盖，那么说明不需要next，next=addr
        alloc_init_pud(pgdp, addr, next, phys,
                prot, alloc_pgtable, flags);
        phys += next - addr;
    } while (pgdp++, addr = next, addr != end);
}

static unsigned long early_pgtable_alloc(void)
{
       unsigned long phys;

       phys = get_free_page();
       memset((void *)phys, 0, PAGE_SIZE);

       return phys;
}

static void create_identical_mapping(void)
{
    unsigned long start;
    unsigned long end;

    /* 创建 .text 代码段 */
    start = (unsigned long) &_text_boot;    //链接器中定义的代码段的起始地址
    end = (unsigned long) &_etext;  //链接器中定义的代码段的结束地址
    //text代码段的可执行权限是ROX只读可执行的，这里创建的时候会顺便把pgd页也分配了
    __create_pgd_mapping((pgd_t *)idmap_pg_dir, start, start,
                    end - start, PAGE_KERNEL_ROX,
                    early_pgtable_alloc,
                    0);

    /* 创建 512 MB 内存的页表 */
    /*map memory*/
       start = PAGE_ALIGN((unsigned long)_etext);
       end = TOTAL_MEMORY;
       __create_pgd_mapping((pgd_t *)idmap_pg_dir, start, start,
                       end - start, PAGE_KERNEL,
                       early_pgtable_alloc,
                       0);
}

/*
作用：创建mmio I/O 映射。
MMIO (Memory-Mapped I/O)：将硬件设备的寄存器映射到内存地址空间，CPU 可以通过读写内存的方式访问硬件设备。
MMIO 区域示例
地址范围              设备类型
0x08000000 - UART     串口控制器
0x10000000 - GPIO     GPIO 控制器
0x20000000 - TIMER    定时器
...
*/
static void create_mmio_mapping(void)
{
       __create_pgd_mapping((pgd_t *)idmap_pg_dir, PBASE, PBASE,
                       DEVICE_SIZE, PROT_DEVICE_nGnRnE,
                       early_pgtable_alloc,
                       0);
}

/*
作用：配置 MMU（内存管理单元）相关的系统寄存器，为启用 MMU 做准备。
*/
static void cpu_init(void)
{
       unsigned long mair = 0;
       unsigned long tcr = 0;
       unsigned long tmp;
       unsigned long parang;

       asm("tlbi vmalle1");//刷新所有 EL1 级别的 TLB 条目
       dsb(nsh);

       write_sysreg(3UL << 20, cpacr_el1);//启用浮点和 SIMD 指令支持
       write_sysreg(1 << 12, mdscr_el1);//配置调试相关功能

       mair = MAIR(0x00UL, MT_DEVICE_nGnRnE) |
              MAIR(0x04UL, MT_DEVICE_nGnRE) |
              MAIR(0x0cUL, MT_DEVICE_GRE) |
              MAIR(0x44UL, MT_NORMAL_NC) |
              MAIR(0xffUL, MT_NORMAL) |
              MAIR(0xbbUL, MT_NORMAL_WT);
       write_sysreg(mair, mair_el1);//配置 MAIR_EL1（内存属性指示器）

       //TxSZ：设置虚拟地址大小（通常 VA_BITS=48）
       //TG_FLAGS：设置页表粒度（通常为 4KB），决定了 48 bit的地址位怎么划分、需要多少级页表
       tcr = TCR_TxSZ(VA_BITS) | TCR_TG_FLAGS;

       tmp = read_sysreg(ID_AA64MMFR0_EL1);
       parang = tmp & 0xf;
       if (parang > ID_AA64MMFR0_PARANGE_48)
               parang = ID_AA64MMFR0_PARANGE_48;

       tcr |= parang << TCR_IPS_SHIFT;//配置物理地址范围(也是48bits)

       write_sysreg(tcr, tcr_el1);//把上面的tcr配置全部写入tcr_el1寄存器
}

static int enable_mmu(void)
{
       unsigned long tmp;
       int tgran4;

       tmp = read_sysreg(ID_AA64MMFR0_EL1);
       tgran4 = (tmp >> ID_AA64MMFR0_TGRAN4_SHIFT) & 0xf;
       if (tgran4 != ID_AA64MMFR0_TGRAN4_SUPPORTED)
               return -1;

       write_sysreg(idmap_pg_dir, ttbr0_el1);   //将L0页表基地址存到TTBR0_EL1寄存器中，这样后面发生缺页的时候，创建映射才会拿得到页表基地址
       isb();

       write_sysreg(SCTLR_ELx_M, sctlr_el1);//SCTLR_ELx_M MMU 使能位
       isb();
       asm("ic iallu");
       dsb(nsh);
       isb();

       return 0;
}

void paging_init(void)
{
        memset(idmap_pg_dir, 0, PAGE_SIZE);//创建第一页 pgd 页，其中idmap_pg_dir就是基地址，需要填到TTBR中
        create_identical_mapping();//创建text段的内存映射，并创建用于测试的 512 MB 内存的页表映射
        create_mmio_mapping();//创建mmio I/O 映射。
        cpu_init();//配置CPU启动MMU的各种寄存器配置，比如页表粒度（4KB）、va/pa地址范围（48bits）、内存属性...
        enable_mmu();//使能mmu使能位、填充L0页表基地址到TTBR0中
        printk("enable mmu done\n");
}