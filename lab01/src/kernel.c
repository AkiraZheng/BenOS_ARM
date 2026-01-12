#include "uart.h"
#include "memset.h"

extern void ldr_test(void);
extern void my_memcpy_test(void);
// extern void __memset_16bytes(void *dest, unsigned long data, int n);

extern unsigned long func_addr[];
extern unsigned long func_num_syms;
extern char func_string[];
static int print_func_name(unsigned long addr)
{
	int i;
	char *p, *string;

	for (i = 0; i < func_num_syms; i++) {
		if (addr == func_addr[i])
			goto found;
	}

	return 0;

found:
    p = &func_string;

    while (1) {
    	p++;

    	if (*p == '\0')
    		i--;

    	if (i == 0) {
    		p++;
    		string = p;
		printk("<0x%lx> %s\n", addr, string);
    		break;
    	}
    }

    return 0;
}

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
	init_printk_done();
	uart_send_string("Welcome BenOS!\r\n");
	printk("printk init done\n");

	//test of ldr
	//my_ldr_test();

	/* 汇编器lab1：查表 */
	print_func_name(0x800880);

	while (1) {
		uart_send(uart_recv());
	}
}
