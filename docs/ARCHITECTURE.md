# SYNAPSE SO - Kernel Architecture

## Overview

SYNAPSE SO kernel is a lightweight, efficient kernel designed for gaming, privacy, and multi-platform compatibility. This document describes the initial kernel architecture implemented in Phase 1.

## Architecture Goals

1. **Lightweight**: Minimal footprint to run on low-end hardware
2. **Performance-focused**: Optimized for gaming workloads
3. **Secure**: Hardened design with continuous auditing
4. **Portable**: Multi-platform support through abstraction layers
5. **Well-documented**: Clear architecture for contributions

## Current Implementation (Phase 1)

### Boot Process

```
BIOS/UEFI
    ↓
GRUB Bootloader (Multiboot compliant)
    ↓
boot.asm - Sets up stack and transfers control
    ↓
kernel_main() - Kernel entry point
```

The kernel uses the Multiboot protocol for compatibility with standard bootloaders like GRUB.

### Kernel Components

#### 1. Boot Code (`boot/`)
- **boot.asm**: Multiboot header and kernel entry point
  - Validates Multiboot magic number
  - Sets up 16KB stack
  - Transfers control to kernel_main()

- **linker.ld**: Kernel memory layout
  - Kernel loaded at 1MB physical address
  - Text, data, and BSS sections aligned to 4KB pages

#### 2. Kernel Core (`kernel/`)

##### VGA Driver (`vga.c`)
- Text mode (80x25) display driver
- Color support (16 colors, foreground and background)
- Functions:
  - `vga_clear_screen()`: Clear display
  - `vga_set_color()`: Set text colors
  - `vga_print()`: Print strings
  - `vga_print_dec()`: Print decimal numbers
  - `vga_print_hex()`: Print hexadecimal numbers

##### Global Descriptor Table (`gdt.c`)
- Memory protection and segmentation
- Sets up 5 GDT entries:
  1. Null segment (required by x86)
  2. Kernel code segment (ring 0, execute/read)
  3. Kernel data segment (ring 0, read/write)
  4. User code segment (ring 3, execute/read)
  5. User data segment (ring 3, read/write)
- Configured for flat memory model (4GB address space)

##### Interrupt Descriptor Table (`idt.c`)
- Interrupt and exception handling
- 256 interrupt handlers defined
- Exception handlers (ISRs 0-31) for:
  - Divide by zero, page faults, general protection faults
  - Double faults, segmentation violations
  - All standard x86 exceptions
- Interrupt service routines (`isr.asm`):
  - Assembly stubs that save processor state
  - Call C handler for processing
  - Restore state and return

##### Main Kernel (`kernel.c`)
- Kernel entry point and initialization
- Initialization sequence:
  1. Clear screen and display banner
  2. Validate Multiboot information
  3. Initialize GDT (memory protection)
  4. Initialize IDT (interrupt handling)
  5. Display system memory information
  6. Enter idle loop (HLT instruction)

#### 3. Support Libraries (`kernel/lib/`)

##### String Library (`string.c`)
- Basic string operations:
  - `strlen()`: String length
  - `strcmp()`: String comparison
  - `strcpy()`: String copy

## Memory Layout

```
Physical Address Space:
0x00000000 - 0x000FFFFF:  Reserved (BIOS, IVT, BDA)
0x00100000 - 0x00FFFFFF:  Kernel (1MB - 16MB)
0x01000000 - 0xFFFFFFFF:  Available for future use
```

## Build System

The Makefile provides:
- **all**: Build kernel and ISO image
- **run**: Launch kernel in QEMU
- **debug**: Launch kernel with debug output
- **clean**: Remove build artifacts
- **rebuild**: Clean and rebuild

Build tools required:
- GCC (with 32-bit support)
- NASM assembler
- GNU LD linker
- GRUB mkrescue (for ISO generation)

## Future Phases (Roadmap)

### Phase 2: Memory Management & Scheduler
- Physical memory manager (frame allocator)
- Virtual memory manager (paging)
- Kernel heap allocator
- Basic task scheduler
- ELF binary loader

### Phase 3: POSIX Userland
- System call interface
- Process management
- Filesystem (VFS layer)
- Basic Unix utilities
- Shell

### Phase 4: Gaming Optimizations
- Real-time scheduler
- GPU driver framework
- Graphics subsystem
- Input system
- Game API compatibility layer

### Phase 5: Security Hardening
- ASLR implementation
- Stack canaries
- NX/DEP support
- Secure boot integration
- Access control lists
- Audit logging system

## Design Principles

1. **Modularity**: Each component is independent and replaceable
2. **Security First**: All code follows secure coding practices
3. **Performance**: Optimized for low-latency gaming workloads
4. **Simplicity**: Clean, readable code for easy auditing
5. **Standards Compliance**: Follows POSIX and open standards

## Conventions

- **Language**: C for kernel code, Assembly for low-level operations
- **Formatting**: 4-space indentation, consistent style
- **Comments**: Function and structure documentation
- **Error Handling**: Graceful degradation where possible
- **License**: GPLv3 - all code is free and open source

## Testing

The kernel can be tested in several ways:
1. **QEMU**: `make run` for virtual testing
2. **Debug mode**: `make debug` for detailed output
3. **Real hardware**: Boot ISO on x86_64 machine

## Contributing

When contributing:
1. Follow existing code style and conventions
2. Add comments for complex logic
3. Update documentation for new features
4. Test on both QEMU and real hardware
5. Ensure GPLv3 license compliance

---

*Last Updated: Phase 1 Implementation*
