.global ldr_test
ldr_test:
    mov x1, 0x80000 // address 0x80000 in x1
    mov x3, 16 // Load immediate decimal value 16 into x3

    ldr x0, [x1] // Load the value from memory address in x1 (0x80000) into x0

    ldr x2, [x1, #8] // Load the value from memory address (x1 + 8) into x2

    ldr x4, [x1, x3] // Load the value from memory address (x1 + x3) into x4, it's 0x80010

    ldr x5, [x1, x3, LSL #3] // Load the value from memory address (x1 + (x3 << 2)) into x5, it's 0x80040

    ret