
#define HCR_EL2_VM      (1UL << 0)  /* VM : Enables stage 2 address translation */
#define HCR_EL2_PTW     (1UL << 2)  /* PTW : protect table walk*/
#define HCR_EL2_FMO     (1ul << 3)  /* FMO : physical firq routing*/
#define HCR_EL2_IMO     (1UL << 4)  /* IMO : physical irq routing*/
#define HCR_EL2_AMO     (1UL << 5) /* AMO : Physical SError interrupt routing.*/

#define HCR_EL2_BSU_IS    (1UL << 10)  /* BSU_IS : Barrier Shareability upgrade*/
#define HCR_EL2_BSU_OS    (2UL << 10)
#define HCR_EL2_BSU_FS    (3UL << 10)

#define HCR_EL2_FB        (1UL << 9)  /* FB : force broadcast when do some tlb ins*/

#define HCR_RW          (1UL << 31)


#define HCR_HOST_NVHE_FLAGS (HCR_RW | HCR_EL2_VM)

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
