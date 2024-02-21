#ifndef BENOS_VIRT_H
#define BENOS_VIRT_H

void jump_to_vm(void);
extern unsigned long hvc_call(int nr, ...);

#endif
