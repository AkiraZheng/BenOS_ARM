#ifndef __MEMSET_H__
#define __MEMSET_H__

typedef unsigned int size_t;

void *__memset_16bytes(void *s, unsigned long val, unsigned long count);
void *my_memset(void *s, int c, size_t count);

#endif
