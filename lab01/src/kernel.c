#include "uart.h"
#include "memset.h"

extern void ldr_test(void);
extern void my_memcpy_test(void);
// extern void __memset_16bytes(void *dest, unsigned long data, int n);

void my_ldr_test(void)
{
	ldr_test(); // Call the external assembly function
	my_memcpy_test();//memcpy test
	// __memset_16bytes((void*)0x200000, 0x5555555555555555, 128);//memset test
	my_memset((void*)0x200000, 0x55, 128);//memset test
	my_memset((void*)0x200004, 0xAA, 102);
}

void kernel_main(void)
{
	uart_init();
	uart_send_string("Welcome BenOS!\r\n");

	//test of ldr
	my_ldr_test();

	while (1) {
		uart_send(uart_recv());
	}
}
