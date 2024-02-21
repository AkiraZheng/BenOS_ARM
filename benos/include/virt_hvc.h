#ifndef VIRT_HVC_H
#define VIRT_HVC_H

#define HVC_CALL(which, arg0, arg1, arg2) ({			\
	register unsigned long a0 asm ("x0") = (unsigned long)(arg0);	\
	register unsigned long a1 asm ("x1") = (unsigned long)(arg1);	\
	register unsigned long a2 asm ("x2") = (unsigned long)(arg2);	\
	register unsigned long a8 asm ("x8") = (unsigned long)(which);	\
	asm volatile ("hvc #0"					\
		      : "+r" (a0)				\
		      : "r" (a1), "r" (a2), "r" (a8)		\
		      : "memory");				\
	a0;							\
})

#define HVC_CALL_0(which) HVC_CALL(which, 0, 0, 0)
#define HVC_CALL_1(which, arg0) HVC_CALL(which, arg0, 0, 0)
#define HVC_CALL_2(which, arg0, arg1) HVC_CALL(which, arg0, arg1, 0)

#endif


