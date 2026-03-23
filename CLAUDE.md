# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BenOS is an ARM64 bare metal operating system for learning ARM64 system programming. It targets Raspberry Pi 3 and 4 boards, implementing kernel boot, exception handling, interrupt management (GIC-400), generic timer, and MMU page tables.

## Build Commands

Build for Raspberry Pi 3 (default):
```bash
cd benos && make
```

Build for Raspberry Pi 4:
```bash
cd benos && make board=rpi4
```

Clean build artifacts:
```bash
cd benos && make clean
```

Run in QEMU (needs to be built first):
```bash
cd benos && make run
```

Debug with GDB:
```bash
cd benos && make debug
```

Then in another terminal:
```bash
gdb-multiarch --tui build/benos.elf
(gdb) target remote localhost:1234
```

## Hardware Debugging

For JTAG debugging on real hardware, use OpenOCD with SEGGER J-Link. See [tools/jlink/README.md](tools/jlink/README.md) for pin mappings and setup instructions.

## Architecture

### Boot Sequence
1. [boot.S](benos/src/boot.S) - Entry point at EL3/EL2, drops to EL1
2. Sets up UART for early output
3. Configures system registers (hcr_el2, sctlr_el1)
4. Sets exception vector table to vbar_el1
5. Clears BSS and calls [kernel_main](benos/src/kernel.c)

### Memory Layout (from [linker.ld](benos/src/linker.ld))
- `0x80000` - Kernel entry point (`.text.boot`)
- `.text` - Code section
- `.rodata` - Read-only data (4KB aligned)
- `.data` - Data section with `idmap_pg_dir` (identity mapping page table)
- `.bss` - BSS section
- `init_pg_dir` - Initial page table directory (4KB aligned)

### Exception Handling
- [entry.S](benos/src/entry.S) - Exception vector table and context save/restore
- Exception vectors at `vectors` label, loaded to `vbar_el1`
- `kernel_entry`/`kernel_exit` - Save/restore pt_regs struct
- IRQ handling calls [irq_handle](benos/src/irq.c)

### Key Components
- **MMU**: [mmu.c](benos/src/mmu.c), [dump_pgtable.c](benos/src/dump_pgtable.c) - Page table management
- **GIC**: [gic_v2.c](benos/src/gic_v2.c) - ARM Generic Interrupt Controller v2
- **Timer**: [timer.c](benos/src/timer.c) - Generic timer implementation
- **UART**: [pl_uart.c](benos/src/pl_uart.c) - PL011 UART driver
- **Print**: [printk.c](benos/src/printk.c) - Kernel printf-like function

### Include Headers
- `benos/include/` - Main headers
- `benos/include/asm/` - Architecture-specific definitions (page tables, system registers, memory types)

## Cross-Compilation

Uses `aarch64-linux-gnu-gcc` toolchain. The `-g3` flag enables full debug info including macro definitions for GDB debugging.

## Running on Real Hardware

Copy `benos.bin` and config from `tools/pi_boot_fw/` to SD card boot partition. The kernel loads at `0x80000`.
