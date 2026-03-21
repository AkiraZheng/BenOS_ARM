
//memset(void *s, int c, size_t n): set n bytes to 0x55
// 0x55 is 1 byte
.global __memset_16bytes
__memset_16bytes:
    // x4 跟第三个传参（x2）的 n 进行对别，作为判断条件
    mov x4, #0

//loop: 一次设置 16 bytes
1:
    stp x1, x1, [x0], #16 //一次写入 16 bytes 到内存
    add x4, x4, #16 //每次增加 16 bytes
    cmp x4, x2 //判断是否达到 n bytes
    b.ne 1b
    
    ret