
#define HCR_EL2_VM      (1UL << 0)  /* VM : Enables stage 2 address translation */
#define HCR_EL2_PTW     (1UL << 2)  /* PTW : protect table walk*/
#define HCR_EL2_FMO     (1ul << 3)  /* FMO : physical firq routing*/
#define HCR_EL2_IMO     (1UL << 4)  /* IMO : physical irq routing*/
#define HCR_EL2_AMO     (1UL << 5) /* AMO : Physical SError interrupt routing.*/
#define HCR_EL2_VI      (1UL << 7)

#define HCR_EL2_BSU_IS    (1UL << 10)  /* BSU_IS : Barrier Shareability upgrade*/
#define HCR_EL2_BSU_OS    (2UL << 10)
#define HCR_EL2_BSU_FS    (3UL << 10)

#define HCR_EL2_FB        (1UL << 9)  /* FB : force broadcast when do some tlb ins*/

#define HCR_RW          (1UL << 31)

#define HCR_HOST_NVHE_FLAGS (HCR_RW | HCR_EL2_VM | HCR_EL2_IMO | HCR_EL2_FB | HCR_EL2_PTW | HCR_EL2_AMO | HCR_EL2_BSU_IS)

#define SCTLR_ELx_C     (1<<2) /*data cache enable*/
#define SCTLR_ELx_M	(1<<0)

#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)
#define SCTLR_MMU_DISABLED   (0 << 0)
#define SCTLR_VALUE_MMU_DISABLED (SCTLR_MMU_DISABLED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_EOE_LITTLE_ENDIAN )

#define SPSR_MASK_ALL (7 << 6)
#define SPSR_EL1h (5 << 0)
#define SPSR_EL2h (9 << 0)
#define SPSR_EL1 (SPSR_MASK_ALL | SPSR_EL1h)
#define SPSR_EL2 (SPSR_MASK_ALL | SPSR_EL2h)

#define CurrentEL_EL1 (1 << 2)
#define CurrentEL_EL2 (2 << 2)
#define CurrentEL_EL3 (3 << 2)

/*
 * 在带参数的宏，#号作为一个预处理运算符,
 * 可以把记号转换成字符串
 *
 * 下面这句话会在预编译阶段变成：
 *  asm volatile("mrs %0, " "reg" : "=r" (__val)); __val; });
 */
#define read_sysreg(reg) ({ \
		unsigned long _val; \
		asm volatile("mrs %0," #reg \
		: "=r"(_val)); \
		_val; \
})

#define write_sysreg(val, reg) ({ \
		unsigned long _val = (unsigned long)val; \
		asm volatile("msr " #reg ", %x0" \
		:: "rZ"(_val)); \
})

/* TCR_EL2 Registers bits */
#define TCR_EL2_RES1		((1 << 31) | (1 << 23))
#define TCR_EL2_TBI		(1 << 20)
#define TCR_EL2_PS_SHIFT	16
#define TCR_EL2_PS_MASK		(7 << TCR_EL2_PS_SHIFT)
#define TCR_EL2_PS_40B		(2 << TCR_EL2_PS_SHIFT)
#define TCR_EL2_TG0_MASK	TCR_TG0_MASK
#define TCR_EL2_SH0_MASK	TCR_SH0_MASK
#define TCR_EL2_ORGN0_MASK	TCR_ORGN0_MASK
#define TCR_EL2_IRGN0_MASK	TCR_IRGN0_MASK
#define TCR_EL2_T0SZ_MASK	0x3f
#define TCR_EL2_MASK	(TCR_EL2_TG0_MASK | TCR_EL2_SH0_MASK | \
			 TCR_EL2_ORGN0_MASK | TCR_EL2_IRGN0_MASK | TCR_EL2_T0SZ_MASK)

/* VTCR_EL2 Registers bits */
#define VTCR_EL2_RES1		(1U << 31)
#define VTCR_EL2_HD		(1 << 22)
#define VTCR_EL2_HA		(1 << 21)
#define VTCR_EL2_PS_SHIFT	TCR_EL2_PS_SHIFT
#define VTCR_EL2_PS_MASK	TCR_EL2_PS_MASK
#define VTCR_EL2_TG0_MASK	TCR_TG0_MASK
#define VTCR_EL2_TG0_SHIFT      TCR_TG0_SHIFT
#define VTCR_EL2_TG0_4K		TCR_TG0_4K
#define VTCR_EL2_TG0_16K	TCR_TG0_16K
#define VTCR_EL2_TG0_64K	TCR_TG0_64K
#define VTCR_EL2_SH0_MASK	TCR_SH0_MASK
#define VTCR_EL2_SH0_INNER	TCR_SH0_INNER
#define VTCR_EL2_ORGN0_MASK	TCR_ORGN0_MASK
#define VTCR_EL2_ORGN0_WBWA	TCR_ORGN0_WBWA
#define VTCR_EL2_IRGN0_MASK	TCR_IRGN0_MASK
#define VTCR_EL2_IRGN0_WBWA	TCR_IRGN0_WBWA
#define VTCR_EL2_SL0_SHIFT	6
#define VTCR_EL2_SL0_MASK	(3 << VTCR_EL2_SL0_SHIFT)
#define VTCR_EL2_T0SZ_MASK	0x3f
#define VTCR_EL2_VS_SHIFT	19
#define VTCR_EL2_VS_8BIT	(0 << VTCR_EL2_VS_SHIFT)
#define VTCR_EL2_VS_16BIT	(1 << VTCR_EL2_VS_SHIFT)

#define VTCR_EL2_T0SZ(x)	TCR_T0SZ(x)

/* 4KB */
#define VTCR_EL2_TGRAN			VTCR_EL2_TG0_4K
#define VTCR_EL2_TGRAN_SL0_BASE		2UL


#define ID_AA64MMFR0_PARANGE_32		0x0
#define ID_AA64MMFR0_PARANGE_40		0x2
#define ID_AA64MMFR0_PARANGE_42		0x3
#define ID_AA64MMFR0_PARANGE_44		0x4
#define ID_AA64MMFR0_PARANGE_48		0x5
#define ID_AA64MMFR0_PARANGE_52		0x6
