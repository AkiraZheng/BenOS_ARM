#include "sysregs.h"

extern char gos_vectors[];

void gos_main()
{
	write_sysreg(gos_vectors, vbar_el1);

	gos_paging_init();
}
