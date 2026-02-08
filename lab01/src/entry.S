#define BAD_SYNC 0
#define BAD_IRQ  1
#define BAD_FIQ  2
#define BAD_ERROR 3

/*
* 定义 inv_entry 宏，用于生成无效异常处理程序
* 参数：
*   el: 异常发生时的异常级别（0 或 1）
*   reason: 异常原因代码
 */
    .macro inv_entry el, reason
    //kernel_entry el //上下文保存-未实现
    mov x0, sp
    mov x1, #\reason
    mrs x2, esr_el1
    b bad_mode //在kernel.c的函数中定义异常处理函数bad_mode
    .endm

/*
* 定义 vtentry 宏，用于生成异常向量表的表项，每个表项占 128 字节
* 参数：
*   label: 异常处理程序的标签
 */
    .macro vtentry label
    .align 7         // 128 字节对齐
    b \label
    .endm

/*
* 异常向量表
* ARM 的异常向量表一共占 2048 字节（4*4*128）
* 因此用 align 11 表示 2048 字节对齐
* 分为 4 组，根据四个异常类型每组有4个表项，每个表项占 128 字节
 */

.align 11
.global vectors
vectors:
    /*
    * Current EL with SP0
    * 异常描述：当前系统运行在EL1时使用EL0的栈指针SP
     */
    vtentry el1_sync_invalid
    vtentry el1_irq_invalid
    vtentry el1_fiq_invalid
    vtentry el1_serror_invalid

    /*
    * Current EL with SPx
    * 异常描述：当前系统运行在EL1时使用当前EL1的栈指针SP
    *          说明是在内核态下发生的异常
    * 当前我们只实现此处的 IRQ 中断
     */
    vtentry el1_sync_invalid
    vtentry el1_irq_invalid
    vtentry el1_fiq_invalid
    vtentry el1_serror_invalid

    /*
    * Lower EL using AArch64
    * 异常描述：在用户态下发生的异常
     */
    vtentry el0_sync_invalid
    vtentry el0_irq_invalid 
    vtentry el0_fiq_invalid
    vtentry el0_serror_invalid

    /*
    * Lower EL using AArch32
    * 异常描述：在用户态下发生的异常
    */
    vtentry el0_sync_invalid
    vtentry el0_irq_invalid
    vtentry el0_fiq_invalid
    vtentry el0_serror_invalid

el1_sync_invalid:
    inv_entry 1, BAD_SYNC
el1_irq_invalid:
    inv_entry 1, BAD_IRQ
el1_fiq_invalid:
    inv_entry 1, BAD_FIQ
el1_serror_invalid:
    inv_entry 1, BAD_ERROR
el0_sync_invalid:
    inv_entry 0, BAD_SYNC
el0_irq_invalid:
    inv_entry 0, BAD_IRQ
el0_fiq_invalid:
    inv_entry 0, BAD_FIQ
el0_serror_invalid:
    inv_entry 0, BAD_ERROR

// // entry.S
// string_test:		//83004
//     .string "t"		//t 和 \0 分别占用83004和83005两个字节
//     // .align 2        //对齐到4字节地址，使代码执行到trigger_alignment时不触发异常

.global trigger_alignment
trigger_alignment:		 //执行到这里变差83006，是未对齐的地址，触发异常
    // 故意触发指令对齐异常
    //在 ARM v8 中，指令通常是 4 字节对齐的
    // 而0x83006不是4的倍数，所以访问的是没有4字节对齐的地址，进而触发对齐异常
    ldr x0, =0x80002
    //ldr x1, [x0]
    str wzr, [x0]
    ret