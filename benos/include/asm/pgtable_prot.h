#ifndef ASM_PGTABLE_PROT_H
#define ASM_PGTABLE_PROT_H

#include <type.h>
#include <asm/pgtable_hwdef.h>

/*
 * Software defined PTE bits definition.
 */
#define PTE_VALID		(1UL << 0)
#define PTE_WRITE		(PTE_DBM)		 /* same as DBM (51) */
#define PTE_DIRTY		(1UL << 55)
#define PTE_SPECIAL		(1UL << 56)
#define PTE_PROT_NONE		(1UL << 58) /* only when !PTE_VALID */

#define _PROT_DEFAULT	(PTE_TYPE_PAGE | PTE_AF | PTE_SHARED)
#define PROT_DEFAULT (_PROT_DEFAULT)

#define PAGE_KERNEL_RO		((PROT_NORMAL & ~PTE_WRITE) | PTE_RDONLY)
#define PAGE_KERNEL_ROX		((PROT_NORMAL & ~(PTE_WRITE | PTE_PXN)) | PTE_RDONLY)
#define PAGE_KERNEL_EXEC	(PROT_NORMAL & ~PTE_PXN)

#define PROT_DEVICE_nGnRnE	(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRnE))
#define PROT_DEVICE_nGnRE	(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_DEVICE_nGnRE))
#define PROT_NORMAL_NC		(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_NC))
#define PROT_NORMAL_WT		(PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL_WT))
#define PROT_NORMAL (PROT_DEFAULT | PTE_PXN | PTE_UXN | PTE_DIRTY | PTE_WRITE | PTE_ATTRINDX(MT_NORMAL))

#define PAGE_KERNEL PROT_NORMAL

#define SWAPPER_PMD_FLAGS       (PMD_TYPE_SECT | PMD_SECT_AF | PMD_SECT_S)
#define SWAPPER_MM_MMUFLAGS     (PMD_ATTRINDX(MT_NORMAL) | SWAPPER_PMD_FLAGS)


/* S2 page prot*/
#define S2_MEMATTR_DEV_nGnRnE		(0b0000 << 2)
#define S2_MEMATTR_DEV_nGnRE		(0b0001 << 2)
#define S2_MEMATTR_DEV_nGRE		(0b0010 << 2)
#define S2_MEMATTR_DEV_GRE		(0b0011 << 2)

#define S2_MEMATTR_NORMAL_WB		(0b1111 << 2)
#define S2_MEMATTR_NORMAL_NC		(0b0101 << 2)
#define S2_MEMATTR_NORMAL_WT		(0b1010 << 2)

#define S2_DES_PAGE			(0b11 << 0)

#define S2_CONTIGUOUS			(1UL << 52)
#define S2_XN				(1UL << 54)
#define S2_AF				(1UL << 10)

#define S2_PFNMAP			(1UL << 55)	// bit 55 - 58 is for software use
#define S2_DEVMAP			(1UL << 56)	// bit 55 - 58 is for software use
#define S2_SHARED			(1UL << 57)	// bit 55 - 58 is for software use

#define S2_SH_NON			(0b00 << 8)
#define S2_SH_OUTER			(0b10 << 8)
#define S2_SH_INNER			(0b11 << 8)

#define S2_AP_NON			(0b00 << 6)
#define S2_AP_RO			(0b01 << 6)
#define S2_AP_WO			(0b10 << 6)
#define S2_AP_RW			(0b11 << 6)

#define S2_PAGE_NORMAL			(S2_DES_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WB)
#define S2_PAGE_NORMAL_NC		(S2_DES_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_NC)
#define S2_PAGE_WT			(S2_DES_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WT)
#define S2_PAGE_DEVICE			(S2_DES_PAGE | S2_AF | S2_SH_OUTER | S2_MEMATTR_DEV_nGnRnE | S2_XN)

#endif /*ASM_PGTABLE_PROT_H*/

