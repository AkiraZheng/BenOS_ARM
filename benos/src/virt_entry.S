#include "sysregs.h"
#include "esr.h"

#define S_FRAME_SIZE 272 /* sizeof(struct pt_regs)	// */
#define S_X0 0 /* offsetof(struct pt_regs, regs[0])	// */
#define S_X1 8 /* offsetof(struct pt_regs, regs[1])	// */
#define S_X2 16 /* offsetof(struct pt_regs, regs[2])	// */
#define S_X3 24 /* offsetof(struct pt_regs, regs[3])	// */
#define S_X4 32 /* offsetof(struct pt_regs, regs[4])	// */
#define S_X5 40 /* offsetof(struct pt_regs, regs[5])	// */
#define S_X6 48 /* offsetof(struct pt_regs, regs[6])	// */
#define S_X7 56 /* offsetof(struct pt_regs, regs[7])	// */
#define S_X8 64 /* offsetof(struct pt_regs, regs[8])	// */
#define S_X10 80 /* offsetof(struct pt_regs, regs[10])	// */
#define S_X12 96 /* offsetof(struct pt_regs, regs[12])	// */
#define S_X14 112 /* offsetof(struct pt_regs, regs[14])	// */
#define S_X16 128 /* offsetof(struct pt_regs, regs[16])	// */
#define S_X18 144 /* offsetof(struct pt_regs, regs[18])	// */
#define S_X20 160 /* offsetof(struct pt_regs, regs[20])	// */
#define S_X22 176 /* offsetof(struct pt_regs, regs[22])	// */
#define S_X24 192 /* offsetof(struct pt_regs, regs[24])	// */
#define S_X26 208 /* offsetof(struct pt_regs, regs[26])	// */
#define S_X28 224 /* offsetof(struct pt_regs, regs[28])	// */
#define S_FP 232 /* offsetof(struct pt_regs, regs[29])	// */
#define S_LR 240 /* offsetof(struct pt_regs, regs[30])	// */
#define S_SP 248 /* offsetof(struct pt_regs, sp)	// */
#define S_PC 256 /* offsetof(struct pt_regs, pc)	// */
#define S_PSTATE 264 /* offsetof(struct pt_regs, pstate)	// */

#define BAD_SYNC        0
#define BAD_IRQ         1
#define BAD_FIQ         2
#define BAD_ERROR       3

#if 1
/* 从VM退出到hypervisor，需要保存上下文到栈里 */
.macro vm_exit el
	sub sp, sp, #S_FRAME_SIZE

	/*
	   保存通用寄存器x0~x29到栈框里pt_regs->x0~x29
	 */
	stp x0, x1, [sp, #16 *0]
	stp x2, x3, [sp, #16 *1]
	stp x4, x5, [sp, #16 *2]
	stp x6, x7, [sp, #16 *3]
	stp x8, x9, [sp, #16 *4]
	stp x10, x11, [sp, #16 *5]
	stp x12, x13, [sp, #16 *6]
	stp x14, x15, [sp, #16 *7]
	stp x16, x17, [sp, #16 *8]
	stp x18, x19, [sp, #16 *9]
	stp x20, x21, [sp, #16 *10]
	stp x22, x23, [sp, #16 *11]
	stp x24, x25, [sp, #16 *12]
	stp x26, x27, [sp, #16 *13]
	stp x28, x29, [sp, #16 *14]

	.if \el == 1
		mrs x21, sp_el1
	.else
		/* x21: 栈顶 的位置*/
		add     x21, sp, #S_FRAME_SIZE
	.endif

	mrs     x22, elr_el2
	mrs     x23, spsr_el2

	/* 把lr保存到pt_regs->lr, 把sp保存到pt_regs->sp位置*/
	stp     lr, x21, [sp, #S_LR]
	/* 把elr_el2保存到pt_regs->pc中
	   把spsr_el2保存到pt_regs->pstate中*/
	stp     x22, x23, [sp, #S_PC]
.endm

/* 从hypervisor 进入到 VM，需要从栈中 恢复 上下文 */
.macro vm_entry el
	/* 从pt_regs->pc中恢复elr_el1,
	   从pt_regs->pstate中恢复spsr_el1
	   */
	ldp     x21, x22, [sp, #S_PC]           // load ELR, SPSR

	.if \el == 1
		ldr x23, [sp, #S_SP]
		msr sp_el1, x23
	.endif

	msr     elr_el2, x21                    // set up the return data
	msr     spsr_el2, x22
	ldp     x0, x1, [sp, #16 * 0]
	ldp     x2, x3, [sp, #16 * 1]
	ldp     x4, x5, [sp, #16 * 2]
	ldp     x6, x7, [sp, #16 * 3]
	ldp     x8, x9, [sp, #16 * 4]
	ldp     x10, x11, [sp, #16 * 5]
	ldp     x12, x13, [sp, #16 * 6]
	ldp     x14, x15, [sp, #16 * 7]
	ldp     x16, x17, [sp, #16 * 8]
	ldp     x18, x19, [sp, #16 * 9]
	ldp     x20, x21, [sp, #16 * 10]
	ldp     x22, x23, [sp, #16 * 11]
	ldp     x24, x25, [sp, #16 * 12]
	ldp     x26, x27, [sp, #16 * 13]
	ldp     x28, x29, [sp, #16 * 14]


	/* 从pt_regs->lr中恢复lr*/
	ldr     lr, [sp, #S_LR]
	add     sp, sp, #S_FRAME_SIZE           // restore sp
	eret
.endm

#else

/* 从VM退出到hypervisor，需要保存上下文到栈里 */
vm_exit:
	sub sp, sp, #S_FRAME_SIZE

	/*
	   保存通用寄存器x0~x29到栈框里pt_regs->x0~x29
	 */
	stp x0, x1, [sp, #16 *0]
	stp x2, x3, [sp, #16 *1]
	stp x4, x5, [sp, #16 *2]
	stp x6, x7, [sp, #16 *3]
	stp x8, x9, [sp, #16 *4]
	stp x10, x11, [sp, #16 *5]
	stp x12, x13, [sp, #16 *6]
	stp x14, x15, [sp, #16 *7]
	stp x16, x17, [sp, #16 *8]
	stp x18, x19, [sp, #16 *9]
	stp x20, x21, [sp, #16 *10]
	stp x22, x23, [sp, #16 *11]
	stp x24, x25, [sp, #16 *12]
	stp x26, x27, [sp, #16 *13]
	stp x28, x29, [sp, #16 *14]

	/* x21: 栈顶 的位置*/
	//add     x21, sp, #S_FRAME_SIZE
	mrs x21, sp_el1

	mrs     x22, elr_el2
	mrs     x23, spsr_el2

	/* 把lr保存到pt_regs->lr, 把sp保存到pt_regs->sp位置*/
	stp     lr, x21, [sp, #S_LR]
	/* 把elr_el2保存到pt_regs->pc中
	   把spsr_el2保存到pt_regs->pstate中*/
	stp     x22, x23, [sp, #S_PC]
	ret

/* 从hypervisor 进入到 VM，需要从栈中 恢复 上下文 */
vm_entry:
	/* 从pt_regs->pc中恢复elr_el1,
	   从pt_regs->pstate中恢复spsr_el1
	   */
	ldp     x21, x22, [sp, #S_PC]           // load ELR, SPSR

        /* */
	ldr x23, [sp, #S_SP]
	msr sp_el1, x23

	msr     elr_el2, x21                    // set up the return data
	msr     spsr_el2, x22
	ldp     x0, x1, [sp, #16 * 0]
	ldp     x2, x3, [sp, #16 * 1]
	ldp     x4, x5, [sp, #16 * 2]
	ldp     x6, x7, [sp, #16 * 3]
	ldp     x8, x9, [sp, #16 * 4]
	ldp     x10, x11, [sp, #16 * 5]
	ldp     x12, x13, [sp, #16 * 6]
	ldp     x14, x15, [sp, #16 * 7]
	ldp     x16, x17, [sp, #16 * 8]
	ldp     x18, x19, [sp, #16 * 9]
	ldp     x20, x21, [sp, #16 * 10]
	ldp     x22, x23, [sp, #16 * 11]
	ldp     x24, x25, [sp, #16 * 12]
	ldp     x26, x27, [sp, #16 * 13]
	ldp     x28, x29, [sp, #16 * 14]


	/* 从pt_regs->lr中恢复lr*/
	ldr     lr, [sp, #S_LR]
	add     sp, sp, #S_FRAME_SIZE           // restore sp
	eret
	ret
#endif

/*
   处理无效的异常向量
 */
	.macro inv_entry el, reason
	mov x0, sp
	mov x1, #\reason
	mrs x2, esr_el1
	b bad_mode
	.endm

/*
   vector table entry
   每个表项是128字节， align 7表示128字节对齐
 */
	.macro vtentry label
	.align 7
	b \label
	.endm

/*
 * Virt Vector Table
 *
 * ARM64的异常向量表一共占用2048个字节
 * 分成4组，每组4个表项，每个表项占128字节
 * 参见ARMv8 spec v8.6第D1.10节
 * align 11表示2048字节对齐
 */
.align 11
.global virt_vectors
virt_vectors:
	/* EL2 with SP0
	   当前系统运行在EL2时 使用EL0的栈指针SP
	   这是一种异常错误的类型
	 */
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid

	/* EL2 with SPx
	 */
	vtentry el2_sync_invalid 
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid

	/* Lower EL using AArch64
	 */
	vtentry __do_vcpu_exit
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid

	/* Lower EL using AArch32
	 */
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid
	vtentry __invalid

__invalid:
	inv_entry 1, BAD_ERROR

__do_vcpu_exit:
	vm_exit 1
	mrs  x25, esr_el2
	lsr     x24, x25, #ESR_ELx_EC_SHIFT     // exception class
	cmp     x24, #ESR_ELx_EC_HVC64
	b.eq    vm_hvc
	
	mov x0, sp
	mov x1, #BAD_SYNC
	mrs x2, esr_el2
	bl bad_mode

vm_hvc:
	mov x0, sp
	bl vm_hvc_handler
	vm_entry 1

el2_sync_invalid:
	vm_exit 2
	mov x0, sp
	mov x1, #BAD_SYNC
	mrs x2, esr_el2
	bl bad_mode


/*
   void jump_to_vs_mode(void)

   跳到 虚拟机中
 */
.global jump_to_vm
jump_to_vm:

	/* The Execution state for EL1 is AArch64 */
	ldr x0, =HCR_HOST_NVHE_FLAGS
	msr hcr_el2, x0

	/* disable MMU for EL1*/
	ldr x0, =SCTLR_VALUE_MMU_DISABLED
	msr sctlr_el1, x0

	/* set SPSR jump to EL1h*/
	ldr x0, =SPSR_EL1
	msr spsr_el2, x0

	/* set VM entry address*/
	adr x0, vm_entry
	msr elr_el2, x0
	
        # set VM's SP
	adr x2, vm_sp
	add  x2, x2, #4096
	msr sp_el1, x2
	
	eret

/* 从这里开始运行VM第一条指令 */
vm_entry:
	ret

.global hvc_call
hvc_call:
	hvc     0x0
	ret

