#ifndef	_P_BASE_H
#define	_P_BASE_H

#ifdef CONFIG_BOARD_PI3B
#define PBASE 0x3F000000
#define ARM_LOCAL_BASE 0x40000000
#define PBASE_END 0x41000000
#else
/* Main peripherals on Low Peripheral mode
 * - ARM and GPU can access
 * see <BCM2711 ARM Peripherals> 1.2.4
 */
#define PBASE 0xFE000000UL
#define PBASE_END 0x100000000UL
/*
 * ARM LOCAL register on Low Peripheral mode
 * - Only ARM can access
 * see <BCM2711 ARM Peripherals> 6.5.2
 */
#define ARM_LOCAL_BASE 0xff800000UL
#define ARM_LOCAL_SIZE 0x80UL
#endif

#define DEVICE_SIZE 0x2000000
#define ARCH_PHYS_OFFSET 0

/* GIC V2*/
#define GIC_V2_DISTRIBUTOR_BASE     (ARM_LOCAL_BASE + 0x00041000)
#define GIC_V2_CPU_INTERFACE_BASE   (ARM_LOCAL_BASE + 0x00042000)
#define GIC_V2_VIR_CONTROL_BASE   (ARM_LOCAL_BASE + 0x00044000)
#define GIC_V2_VCPU_INTERFACE_BASE   (ARM_LOCAL_BASE + 0x00046000)
#define GIC_V2_DISTRIBUTOR_SIZE     (4096)
#define GIC_V2_CPU_INTERFACE_SIZE    (8192)

#endif  /*_P_BASE_H */
