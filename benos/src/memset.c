#include "memset.h"


static void * __memset_1byte(void *s, int c, size_t count)
{
    //对于无法对齐的部分，逐字节设置，纯c语言实现
    char *xs = s;
    while(count--){
        *xs++ = c;
    }

    return s;
}

//s 和 count 要转换成16的倍数对齐，c 要转换成 8 bytes
static void * __memset(char *s, int c, size_t count)
{
    //进行对齐
    char *p = s;
    unsigned long align = 16;
    size_t size, left = count;
    int n,i;
    unsigned long addr = (unsigned long)p;//8 bytes
    unsigned long data = 0ULL;

    // transform c to 8 bytes data(unsigned long)
    //(eg c=0x55 → data=0x5555555555555555)
    for(i = 0; i < 8; i++){
        data |= (((unsigned long)c) & 0xff) << (i * 8);
    }

    //1. check start address is aligned with 16 bytes
    if(addr & (align - 1)){
        //firstly, set the bytes before aligned address
        size = addr & (align - 1);
        __memset_1byte(p, c, size);
        p += size;
        left -= size;
    }

    // align 16 bytes
    //at least 16 bytes need to be set
    if(left >= align){
        n = left / align;
        left = left % align;

        __memset_16bytes(p, data, 16*n);

        if(left){
            __memset_1byte(p + 16*n, c, left);
        }
    }

    return s;
}

void *my_memset(void *s, int c, size_t count){
    return __memset(s, c, count);
}