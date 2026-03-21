/* 设置 HCR 寄存器的标志位：
* - HCR_RW: 用于设置寄存器的读写权限位。
* - HCR_HOST_NVHE_FLAGS: 用于配置非虚拟化主机环境的标志位。
*/

#define HCR_RW          (1UL << 31)
#define HCR_HOST_NVHE_FLAGS  (HCR_RW)

#define SCTLR_ELx_M    (1<<0)

/* 设置 SCTLR 寄存器的标志位：
* - SCTRL_EE_LITTLE_ENDIAN: 设置异常级别为小端模式。
* - SCTRL_EOE_LITTLE_ENDIAN: 设置外部异常为小端模式。
* - SCTRL_MMU_DISABLED: 禁用内存管理单元 (MMU)
* - SCTRL_VALUE_MMU_DISABLED: 组合上述标志以表示 MMU 被禁用的配置。
*/

#define SCTRL_EE_LITTLE_ENDIAN    (0 << 25)
#define SCTRL_EOE_LITTLE_ENDIAN   (0 << 24)
#define SCTRL_MMU_DISABLED        (0 << 0)
#define SCTRL_VALUE_MMU_DISABLED   (SCTRL_EE_LITTLE_ENDIAN | SCTRL_EOE_LITTLE_ENDIAN | SCTRL_MMU_DISABLED)

/* 设置 SPSR 寄存器的标志位：
* - SPSR_MASK_ALL: 用于屏蔽所有中断。
* - SPSR_EL1h: 设置为 EL1 的高权限模式。
* - SPSR_EL2h: 设置为 EL2 的高权限模式。
* - SPSR_EL1: 组合标志以表示进入 EL1 高权限模式。
* - SPSR_EL2: 组合标志以表示进入 EL2 高权限模式。
*/

#define SPSR_MASK_ALL        (0b111 << 6)
#define SPSR_EL1h           (0b0101 << 0)
#define SPSR_EL2h           (0b1001 << 0)
#define SPSR_EL1            (SPSR_MASK_ALL | SPSR_EL1h)
#define SPSR_EL2            (SPSR_MASK_ALL | SPSR_EL2h)

/* 设置 CurrentEL 寄存器的标志位：
* - CurrentEL_EL1: 表示当前处于 EL1。
* - CurrentEL_EL2: 表示当前处于 EL2。
* - CurrentEL_EL3: 表示当前处于 EL3。
*/

#define CurrentEL_EL1       (0b01 << 2)
#define CurrentEL_EL2       (0b10 << 2)
#define CurrentEL_EL3       (0b11 << 2)

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