#include "uart.h"

extern void ldr_test(void);

void my_ldr_test(void)
{
	ldr_test(); // Call the external assembly function
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
