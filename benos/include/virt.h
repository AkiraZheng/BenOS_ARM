#ifndef BENOS_VIRT_H
#define BENOS_VIRT_H

extern unsigned long jump_to_vm(unsigned long gpa_addr);
extern unsigned long hvc_call(int nr, ...);

#endif
