#include <asm/pgtable.h>
#include <asm/pgtable_prot.h>
#include <asm/pgtable_hwdef.h>

extern char idmap_pg_dir[];

struct addr_marker {
       unsigned long start_address;
       char *name;
};

struct prot_bits {
       unsigned long mask;//ATTR 表中的哪一类属性（idx），比如PTE_RDONLY、PTE_AF
       unsigned long val;//ATTR 表中的哪一类属性（idx），比如PTE_RDONLY、PTE_AF
       const char *set;//属性值，表示开启这个属性的话应该打印什么，比如"ro"、"AF"
       const char *clear;//属性值，表示关闭这个属性的话应该打印什么，比如"RW"、" "
};

struct pg_level {
       const struct prot_bits *bits;//数组，数组中每一项表示一种属性及其值。
       const char *name; // 层级名称："PGD"/"PUD"/"PMD"/"PTE"
       size_t num;//页面大小：
       unsigned long mask;//提取当前level entry 项的 prot mask
};

static const struct addr_marker address_markers[] = {
       { 0, "Identical mapping" },
};

//初始化每个页表项的15种属性
static const struct prot_bits pte_bits[] = {
       {
               .mask   = PTE_VALID,
               .val    = PTE_VALID,
               .set    = " ",
               .clear  = "F",
       }, {
               .mask   = PTE_USER,
               .val    = PTE_USER,
               .set    = "USR",
               .clear  = "   ",
       }, {
               .mask   = PTE_RDONLY,
               .val    = PTE_RDONLY,
               .set    = "ro",
               .clear  = "RW",
       }, {
               .mask   = PTE_PXN,
               .val    = PTE_PXN,
               .set    = "NX",
               .clear  = "x ",
       }, {
               .mask   = PTE_SHARED,
               .val    = PTE_SHARED,
               .set    = "SHD",
               .clear  = "   ",
       }, {
               .mask   = PTE_AF,
               .val    = PTE_AF,
               .set    = "AF",
               .clear  = "  ",
       }, {
               .mask   = PTE_NG,
               .val    = PTE_NG,
               .set    = "NG",
               .clear  = "  ",
       }, {
               .mask   = PTE_CONT,
               .val    = PTE_CONT,
               .set    = "CON",
               .clear  = "   ",
       }, {
               .mask   = PTE_TABLE_BIT,
               .val    = PTE_TABLE_BIT,
               .set    = "   ",
               .clear  = "BLK",
       }, {
               .mask   = PTE_UXN,
               .val    = PTE_UXN,
               .set    = "UXN",
       }, {
               .mask   = PTE_ATTRINDX_MASK,
               .val    = PTE_ATTRINDX(MT_DEVICE_nGnRnE),
               .set    = "DEVICE/nGnRnE",
       }, {
               .mask   = PTE_ATTRINDX_MASK,
               .val    = PTE_ATTRINDX(MT_DEVICE_nGnRE),
               .set    = "DEVICE/nGnRE",
       }, {
               .mask   = PTE_ATTRINDX_MASK,
               .val    = PTE_ATTRINDX(MT_DEVICE_GRE),
               .set    = "DEVICE/GRE",
       }, {
               .mask   = PTE_ATTRINDX_MASK,
               .val    = PTE_ATTRINDX(MT_NORMAL_NC),
               .set    = "MEM/NORMAL-NC",
       }, {
               .mask   = PTE_ATTRINDX_MASK,
               .val    = PTE_ATTRINDX(MT_NORMAL),
               .set    = "MEM/NORMAL",
       }
};

//初始化 4 个层级的结构
static struct pg_level pg_level[] = {
       {//这个是占位符，为了让后面的级别从1开始算起
       }, { /* pgd */
               .name   = "PGD",
               .bits   = pte_bits,
               .num    = ARRAY_SIZE(pte_bits),
       }, { /* pud */
               .name   = "PUD",
               .bits   = pte_bits,
               .num    = ARRAY_SIZE(pte_bits),
       }, { /* pmd */
               .name   = "PMD",
               .bits   = pte_bits,
               .num    = ARRAY_SIZE(pte_bits),
       }, { /* pte */
               .name   = "PTE",
               .bits   = pte_bits,
               .num    = ARRAY_SIZE(pte_bits),
       },
};

static void dump_prot(unsigned long prot, const struct prot_bits *bits,
                        size_t num)
{
    unsigned i;

    for (i = 0; i < num; ++i, bits++) {
        const char *s;

        //判断当前val当前属性的prot值跟设置当前属性为true的值是否一样
        //一样：表示设置了该属性，输出bits->set
        //不一样：说明没甚至，输出 null
        if ((prot & bits->mask) == bits->val)
            s = bits->set;
        else
            s = bits->clear;
        
        if (s)
            printk(" %s", s);
    }
}

/*
作用：dump 打印当前叶子 entry 的节点信息
*/
static int print_pgtable(unsigned long start, unsigned long end,
                int level, unsigned long val)
{
    static const char units[] = "KMGT";//叶子节点 entry 代表的内存大小：KB、MB、GB、TB
    unsigned long prot = val & pg_level[level].mask;//取出当前entry的prot属性
    unsigned long delta;
    const char *unit = units;
    int i;

    /*address_markers只有一个entry，所以表示只有start=0的时候才会打印这一行
    * walk_pgd((pgd_t *)idmap_pg_dir, start=0, end=TOTAL_MEMORY);
    * 从上面的调用可以看出，要遍历的范围是从idmap_pg_dir遍历起，遍历范围[0,TOTAL_MEMORY]
    * 因此，只有找到的第一个叶子节点，start 才会是0，也才会打印下面这行字，表示dump的起点 entry
    */
    for (i = 0; i < ARRAY_SIZE(address_markers); i++)
        if (start == address_markers[i].start_address)
            printk("---[ %s ]---\n", address_markers[i].name);

    if (val == 0)
        return 0;

    //打印内存范围
    //本实验中，从 0x80000 text 段的起始地址处开始才是有效的val，因此从0x80000开始才有打印
    printk("0x%016lx-0x%016lx   ", start, end);

    //打印叶子节点的页面大小
    delta = (end - start) >> 10;
    while (!(delta & 1023) && units[1]) {
        delta >>= 10;
        unit++;
    }
    printk("%9lu%c %s", delta, *unit,
                    pg_level[level].name);

    //打印当前 entry 的页表属性
    if (pg_level[level].bits)
        dump_prot(prot, pg_level[level].bits,
                        pg_level[level].num);
    printk("\n");

    return 0;
}

static void pg_level_init()
{
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(pg_level); ++i)//4个页表级别，ARRAY_SIZE(pg_level)=4
        if (pg_level[i].bits)
            for (j = 0; j < pg_level[i].num; ++j)//这里设置了ATTR表的15个属性
                pg_level[i].mask |= pg_level[i].bits[j].mask;
}

static void walk_pte(pmd_t *pmdp, unsigned long start, unsigned long end)
{
       pte_t *ptep = pte_offset_phys(pmdp, start);
       unsigned long addr = start;

       do {
               print_pgtable(addr, addr + PAGE_SIZE, 4, pte_val(*ptep));
       } while (ptep++, addr += PAGE_SIZE, addr != end);
}

static void walk_pmd(pud_t *pudp, unsigned long start, unsigned long end)
{
       unsigned long next, addr = start;
       pmd_t *pmdp = pmd_offset_phys(pudp, start);
       pmd_t pmd;

       do {
            pmd = *pmdp;
            next = pmd_addr_end(addr, end);

            if (pmd_none(pmd) || pmd_sect(pmd))
                    print_pgtable(addr, next, 3, pmd_val(pmd));
            else
                    walk_pte(pmdp, addr, next);
       } while (pmdp++, addr = next, addr != end);
}

static void walk_pud(pgd_t *pgdp, unsigned long start, unsigned long end)
{
       unsigned long next, addr = start;
       pud_t *pudp = pud_offset_phys(pgdp, start);
       pud_t pud;

       do {
            pud = *pudp;
            next = pud_addr_end(start, end);
            
            //pud_sect(pud) == true 表示是叶子节点
            if (pud_none(pud) || pud_sect(pud))
                    print_pgtable(addr, next, 2, pud_val(pud));
            else
            //非叶子节点，向下继续遍历
                    walk_pmd(pudp, addr, next);

       } while (pudp++, addr = next, addr != end);
}

static void walk_pgd(pgd_t *pgd, unsigned long start, unsigned long size)
{
       unsigned long end = start + size;
       unsigned long next, addr = start;
       pgd_t *pgdp;
       pgd_t pgd_entry;

       pgdp = pgd_offset_raw(pgd, start);

       do {
            pgd_entry = *pgdp;
            next = pgd_addr_end(addr, end);

            if (pgd_none(pgd_entry))//无效页，未用于映射的页表项
                    print_pgtable(addr, next, 1, pgd_val(pgd_entry));
            else
                    walk_pud(pgdp, addr, next);
       } while (pgdp++, addr = next, addr != end);
}

void dump_pgtable(void)
{
    //主要是根据pte_bits的属性mask值填充每个level的mask值
    //把PTE的各个属性的mask填进去（填1）
    pg_level_init();
    walk_pgd((pgd_t *)idmap_pg_dir, 0, TOTAL_MEMORY);
}