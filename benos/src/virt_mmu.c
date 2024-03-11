#include "io.h"
#include "sysregs.h"
#include <asm/pgtable.h>
#include "error.h"

/*
 * 本实验采用 “IPA 40Bits + 4KB页面 + 4级映射“ 的方式来创建S2页表。
 *
 * 根页表（root page table）的大小为4KB
 * */
#define S2_ROOT_PG_SIZE  (PAGE_SIZE)
char s2_pg_dir[S2_ROOT_PG_SIZE] __attribute__((aligned(S2_ROOT_PG_SIZE)));

static void flush_all_vms(void)
{
	asm volatile(
		"dsb sy\n"
		"tlbi vmalls12e1is\n"
		"dsb sy\n"
		"isb\n"
		: : : "memory"
	);
}

static unsigned long get_vtcr_el2(unsigned int parange, unsigned int ipa_bits,
		unsigned int pgtable_levels, unsigned int tg)
{
       unsigned long value = 0;
       unsigned long sl0;

       printk("%s PARange: %d IPA bit: %d PageTable levels: %d TG: %d\n", __func__, parange, ipa_bits, pgtable_levels, tg);

       /* 
	* 根据IPA位宽来得到memory region。memory region计算方式： 2^(64-T0SZ) bytes
	*
	* t0sz =  表示 40bits IPA
	* */
	value |= ((64 - ipa_bits) << 0);

	/* 设置页面粒度 : 4K */
	value |= (tg << 14);

	/*
	 * SL0:表示从第几级页表开始查询
	 *
	 * 从ARMv8.6手册，Table D5-13可知：
	 * 对于4KB粒度+40bits IPA来说，可以从level0 开始查找
	 *
	 * SL0 = 0: start with level 2
	 * SL0 = 1: start with level1
	 * SL0 = 2: start with level0
	 * */
	if (pgtable_levels == 4)
		sl0 = 2;
	else
		sl0 = 1;
	value |= (sl0 << 6);


	/* cacheability attribute*/
	value |= (0x1 << 8);	// Normal memory, Inner WBWA
	value |= (0x1 << 10);	// Normal memory, Outer WBWA
	value |= (0x3 << 12);	// Inner Shareable


	// PS --- pysical size 44bit
	value |= (parange << 16);

	// vmid -- 8bit
	value |= (0x0 << 19);

	return value;
}

void write_stage2_pg_reg(void)
{
	unsigned long val;
	unsigned int parange;
	unsigned int pgtable_levels = 3;
	unsigned int ipa_bits;
	unsigned int tg = TG0_4K;  

	parange = read_sysreg(id_aa64mmfr0_el1);
	parange &= 0xf;
	printk("%s PARange 0x%lx\n", __func__, parange);

	if (parange >= ID_AA64MMFR0_PARANGE_44) {
		parange = ID_AA64MMFR0_PARANGE_44;
		ipa_bits = 44;
		pgtable_levels = 4;
	} 

	/* FIX Me: 
	 *
	 * 树莓派4B最大PA size支持40bits，我们这里强制把ipa_bits设置为40bits,
	 * 验证40bits的IPA，能否用4级页表映射*/
	ipa_bits = 40;

	val = get_vtcr_el2(parange, ipa_bits, pgtable_levels, tg);
	write_sysreg(val, vtcr_el2);
	
	write_sysreg(s2_pg_dir, vttbr_el2);

	flush_all_vms();
}

/*
 * 这里使用"44bit IPA + 4级映射" 来创建S2页表
 */
#define S2_IPA_BITS                    44UL

#define S2_PGD_SHIFT			39UL
#define S2_PGD_SIZE			(1UL << S2_PGD_SHIFT)
#define S2_PGD_MASK			(~(S2_PGD_SIZE - 1))
#define S2_PTRS_PER_PGD                 (1 << (S2_IPA_BITS - S2_PGD_SHIFT))

#define S2_PUD_SHIFT			30UL
#define S2_PUD_SIZE			(1UL << S2_PUD_SHIFT)
#define S2_PUD_MASK			(~(S2_PUD_SIZE - 1))
#define S2_PTRS_PER_PUD                 (1 << (S2_PGD_SHIFT - S2_PUD_SHIFT))

#define S2_PMD_SHIFT			21UL
#define S2_PMD_SIZE			(1UL << S2_PMD_SHIFT)
#define S2_PMD_MASK			(~(S2_PMD_SIZE - 1))

#define S2_PTE_SHIFT			12UL
#define S2_PTE_SIZE			(1UL << S2_PTE_SHIFT)
#define S2_PTE_MASK			(~(S2_PTE_SIZE - 1))

#define s2_pud_index(addr) (((addr) >> S2_PUD_SHIFT) & (S2_PTRS_PER_PUD - 1))
#define s2_pud_offset_raw(pgd, addr) ((pud_t *)(((pud_t *)(pgd_page_paddr(*(pgd))) \
			+ s2_pud_index(addr))))

#define s2_pud_offset(pgd, addr) (s2_pud_offset_raw(pgd, (addr)))

#define s2_pgd_index(addr) (((addr) >> S2_PGD_SHIFT) & (S2_PTRS_PER_PGD - 1))
#define s2_pgd_offset_raw(pg_dir, addr) ((pgd_t *)(((pgd_t *)(pg_dir)) \
			+ s2_pgd_index(addr)))

#define s2_pgd_offset(pg_dir, addr) (s2_pgd_offset_raw(pg_dir, (addr)))

#define __pa(x) (x)

static inline unsigned long pmd_alloc_one(void)
{
	return get_free_page();
}

static inline void pud_populate(pud_t *pud, unsigned long pmd_phy)
{
	set_pud(pud, __pud(pmd_phy | PMD_TYPE_TABLE));
}

int __pmd_alloc(pud_t *pud, unsigned long address)
{
	unsigned long pmd_phy = pmd_alloc_one();
	if (!pmd_phy)
		return -ENOMEM;

	if (!pud_present(*pud))
		pud_populate(pud, pmd_phy);

	return 0;
}

static inline unsigned long pte_alloc_one(void)
{
	return get_free_page();
}

static inline void pmd_populate(pmd_t *pmd, unsigned long pte)
{
	set_pmd(pmd, __pmd(__pa(pte) | PMD_TYPE_TABLE));
}

int __pte_alloc(pmd_t *pmd, unsigned long address)
{
	unsigned long pte_phy = pte_alloc_one();
	if (!pte_phy)
		return -ENOMEM;

	if (!pmd_present(*pmd))
		pmd_populate(pmd, pte_phy);

	return 0;
}

pte_t *pte_alloc(pmd_t *pmd, unsigned long address)
{
	return ((pmd_none(*pmd)) && __pte_alloc(pmd, address))?
		NULL: pte_offset_phys(pmd, address);
}

pmd_t *pmd_alloc(pud_t *pud, unsigned long address)
{
	return ((pud_none(*pud)) && __pmd_alloc(pud, address))?
		NULL: pmd_offset_phys(pud, address);
}

static inline unsigned long pud_alloc_one(void)
{
	return get_free_page();
}

static inline void pgd_populate(pgd_t *pgd, unsigned long pud_phy)
{
	set_pgd(pgd, __pgd(pud_phy | PUD_TYPE_TABLE));
}

int __pud_alloc(pgd_t *pgd, unsigned long address)
{
	unsigned long pud_phy = pud_alloc_one();
	if (!pud_phy)
		return -ENOMEM;

	if (!pgd_present(*pgd))
		pgd_populate(pgd, pud_phy);

	return 0;
}

pud_t *pud_alloc(pgd_t *pgd, unsigned long address)
{
	return ((pgd_none(*pgd)) && __pud_alloc(pgd, address))?
		NULL: s2_pud_offset(pgd, address);
}

int stage2_page_fault(unsigned long gpa, unsigned long hpa, unsigned long size, unsigned long prot)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	//printk("%s s2_pgdir 0x%lx\n", __func__, &s2_pg_dir);
	//
	pgd = s2_pgd_offset(&s2_pg_dir, gpa); 
	if (!pgd)
		return -ENOMEM;

	//printk("pgd 0x%lx\n pgd_index 0x%x pgd_val 0x%lx\n", (unsigned long)pgd, s2_pgd_index(gpa), pgd_val(*pgd));

	pud = pud_alloc(pgd, gpa);
	if (!pud)
		return -ENOMEM;

	//printk("pud 0x%lx\n pud_index 0x%x pud_val 0x%lx\n", (unsigned long)pud, s2_pud_index(gpa), pud_val(*pud));

	pmd = pmd_alloc(pud, gpa); 
	if (!pmd)
		return -ENOMEM;

	//printk("pmd 0x%lx\n pmd_index 0x%x pmd_val 0x%x\n", (unsigned long)pmd, pmd_index(gpa), pmd_val(*pmd));

	pte = pte_alloc(pmd, gpa);
	if (!pte)
		return -ENOMEM;

	//printk("pte 0x%lx\n pte_index 0x%x pte_val 0x%x\n", (unsigned long)pte, pte_index(gpa), pte_val(*pte));

	set_pte(pte, pfn_pte(hpa >> PAGE_SHIFT, prot));

	return 0;
}
