#ifndef BENOS_TIMER_H
#define BENOS_TIMER_H

#include "irq.h"

void timer_init(void);
enum irq_res handle_timer_irq(void);

#endif

