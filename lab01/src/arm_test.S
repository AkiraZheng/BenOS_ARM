.global ldr_test
ldr_test:
    mov x1, 0x80000 // address 0x80000 in x1
    mov x3, 16 // Load immediate decimal value 16 into x3

    ldr x0, [x1] // Load the value from memory address in x1 (0x80000) into x0

    ldr x2, [x1, #8] // Load the value from memory address (x1 + 8) into x2

    ldr x4, [x1, x3] // Load the value from memory address (x1 + x3) into x4, it's 0x80010

    ldr x5, [x1, x3, LSL #3] // Load the value from memory address (x1 + (x3 << 2)) into x5, it's 0x80040

    ret

//memcpy: copy 32 bytes from 0x80000 to 0x200000
.global my_memcpy_test
my_memcpy_test:
    mov x0, 0x80000 // source address
    mov x1, 0x200000 // destination address
    add x2, x0, 32 // number of bytes to copy
// loop to copy each byte
1:
    ldr x4, [x0], #8 //ldr then add 8 to x0
    str x4, [x1], #8
    cmp x0, x2
    b.cc 1b // if not done, repeat

    ret