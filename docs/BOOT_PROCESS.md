# SYNAPSE SO - Boot Process

## Overview

This document describes the boot process of SYNAPSE SO from power-on to the kernel initialization.

## Boot Sequence

### 1. BIOS/UEFI Stage

1. **Power-On Self-Test (POST)**
   - CPU initializes in 16-bit real mode
   - BIOS performs hardware checks
   - Memory detection and initialization

2. **Boot Device Selection**
   - BIOS searches for bootable devices
   - Reads Master Boot Record (MBR) from selected device
   - Loads bootloader into memory at `0x7C00`

### 2. GRUB Bootloader Stage

We use GRUB2 as our bootloader, which provides Multiboot specification support.

1. **GRUB Stage 1**
   - Loaded by BIOS at `0x7C00`
   - Loads GRUB Stage 1.5 or Stage 2

2. **GRUB Stage 2**
   - Reads filesystem
   - Loads `grub.cfg` configuration
   - Displays boot menu (if configured)
   - Parses Multiboot header in kernel

3. **Kernel Loading**
   - GRUB reads `boot/kernel.elf`
   - Verifies Multiboot header (magic: `0x1BADB002`)
   - Loads kernel into memory starting at `0x100000` (1MB)
   - Constructs Multiboot information structure

4. **CPU Mode Transition**
   - GRUB switches CPU from 16-bit real mode to 32-bit protected mode
   - Sets up initial GDT (Global Descriptor Table)
   - Disables interrupts
   - Jumps to kernel entry point

### 3. Kernel Entry (`boot/boot.asm`)

The kernel entry point is defined in `boot/boot.asm`:

```asm
section .multiboot
    ; Multiboot header
    MULTIBOOT_MAGIC        equ 0x1BADB002
    MULTIBOOT_FLAGS        equ 0x00000003
    MULTIBOOT_CHECKSUM     equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
    
    align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
    align 16
    stack_bottom:
        resb 16384  ; 16KB stack
    stack_top:

section .text
    global _start
    extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Push Multiboot parameters
    push ebx    ; Multiboot info structure pointer
    push eax    ; Multiboot magic number
    
    ; Call kernel main
    call kernel_main
    
    ; Halt if kernel returns
    cli
.hang:
    hlt
    jmp .hang
```

**Key Points:**
- Stack grows downward from `stack_top`
- EAX contains Multiboot magic (`0x2BADB002`)
- EBX contains pointer to Multiboot info structure
- Interrupts are disabled (`cli`)

### 4. Kernel Initialization (`kernel/kernel.c`)

The `kernel_main()` function performs system initialization:

```c
void kernel_main(uint32_t magic, multiboot_info_t* mbi)
```

#### 4.1 VGA Initialization
```
vga_init() - Initialize 80x25 text mode display
```
- Sets up VGA buffer at `0xB8000`
- Clears screen
- Initializes cursor position

#### 4.2 GDT Setup
```
gdt_init() - Set up Global Descriptor Table
```
- Creates 5 GDT entries:
  - Entry 0: Null segment (required)
  - Entry 1: Kernel code segment (base: 0, limit: 4GB)
  - Entry 2: Kernel data segment (base: 0, limit: 4GB)
  - Entry 3: User code segment (base: 0, limit: 4GB)
  - Entry 4: User data segment (base: 0, limit: 4GB)
- Loads GDT with `lgdt` instruction
- Reloads segment registers

#### 4.3 IDT Setup
```
idt_init() - Set up Interrupt Descriptor Table
```
- Creates 256 IDT entries
- Maps interrupt vectors to handlers
- Configures CPU exceptions (0-31)
- Configures hardware interrupts (32-47)
- Configures system call gate (0x80)
- Loads IDT with `lidt` instruction

#### 4.4 Physical Memory Manager
```
pmm_init(mbi) - Initialize Physical Memory Manager
```
- Parses Multiboot memory map
- Identifies available memory regions
- Creates bitmap for frame allocation
- Each frame = 4KB (PAGE_SIZE)
- Marks kernel memory as used

Memory regions:
```
0x00000000 - 0x000FFFFF:  Reserved (BIOS, IVT, BDA)
0x00100000 - 0x00FFFFFF:  Kernel and core data
0x01000000+:              Available for allocation
```

#### 4.5 Virtual Memory Manager
```
vmm_init() - Initialize Virtual Memory Manager
```
- Allocates kernel page directory
- Sets up page tables
- Identity maps first 4MB (0x0 - 0x400000)
- Maps kernel to higher half (3GB+):
  - Physical: `0x100000` → Virtual: `0xC0100000`
- Enables paging by setting CR0.PG bit
- Loads CR3 with page directory physical address

Page structure:
```
Page Directory (1024 entries) → Page Tables (1024 entries each) → 4KB Pages
```

#### 4.6 Heap Initialization
```
heap_init() - Initialize Kernel Heap
```
- Sets up free list allocator
- Initial heap size: 1MB
- Provides `kmalloc()`, `kfree()`, `krealloc()`
- Automatic expansion via VMM

#### 4.7 Timer Setup
```
timer_init() - Initialize PIT Timer
```
- Configures Programmable Interval Timer (PIT)
- Frequency: 100Hz (10ms per tick)
- Connects to IRQ 0 (interrupt 32)

#### 4.8 System Call Interface
```
syscall_init() - Initialize System Calls
```
- Registers system call handlers
- Configures interrupt 0x80 for syscalls

#### 4.9 Process Management
```
process_init() - Initialize Process Subsystem
```
- Creates idle process (PID 0)
- Initializes process list
- Sets up initial scheduling state

#### 4.10 Scheduler
```
scheduler_init() - Initialize Scheduler
```
- Configures round-robin scheduler
- Sets time quantum (default: 10 ticks)
- Initializes ready queue

## Memory Map After Boot

```
Virtual Address Space (with paging enabled):

0x00000000 - 0x000FFFFF:  Identity mapped (BIOS, low memory)
0x00100000 - 0x003FFFFF:  Identity mapped (kernel physical location)
0x00400000 - 0xBFFFFFFF:  Unmapped (available for user space)
0xC0000000 - 0xC00FFFFF:  Kernel code and data (higher half)
0xC0100000 - 0xDFFFFFFF:  Kernel heap and dynamic allocations
0xE0000000 - 0xEFFFFFFF:  Temporary mappings (256 pages)
0xF0000000 - 0xFFFFFFFF:  Reserved (future use)

Physical Memory:
0x00000000 - 0x000FFFFF:  Reserved
0x00100000 - 0x002FFFFF:  Kernel image
0x00300000 - 0x003FFFFF:  PMM bitmap
0x00400000+:              Available frames
```

## Stack Layout

### Initial Kernel Stack

```
[Higher addresses]
+------------------+
| Multiboot info   | ← EBX (pushed by boot.asm)
+------------------+
| Multiboot magic  | ← EAX (pushed by boot.asm)
+------------------+
| Return address   |
+------------------+
| Saved EBP        | ← ESP after function prologue
+------------------+
| Local variables  |
+------------------+
[Lower addresses]
```

### Process Stack

Each process has its own kernel stack (separate from user stack):
- Size: 8KB per process
- Grows downward
- Stores process context during context switches

## Interrupt Flow

### Hardware Interrupt Example (Timer IRQ 0)

1. **Hardware triggers interrupt**
   - Timer chip sends signal to PIC
   - PIC sends interrupt to CPU

2. **CPU interrupt handling**
   - Saves current state (CS, EIP, EFLAGS)
   - Looks up IDT entry 32 (IRQ 0 → interrupt 32)
   - Jumps to interrupt handler

3. **Assembly stub (`isr.asm`)**
   ```asm
   push 0          ; Error code (dummy for IRQs)
   push 32         ; Interrupt number
   jmp isr_common
   ```

4. **Common handler saves context**
   ```asm
   isr_common:
       pusha       ; Save all registers
       push ds
       push es
       push fs
       push gs
       
       call idt_handler    ; Call C handler
       
       pop gs
       pop fs
       pop es
       pop ds
       popa
       add esp, 8  ; Clean error code and int number
       iret        ; Return from interrupt
   ```

5. **C handler processes interrupt**
   ```c
   void idt_handler(registers_t* regs) {
       if (regs->int_no == 32) {
           timer_tick();
       }
       pic_send_eoi(regs->int_no);
   }
   ```

6. **Return to interrupted code**
   - `iret` restores CS, EIP, EFLAGS
   - Execution continues

## Troubleshooting Boot Issues

### No Output

**Symptom:** Black screen, no text

**Possible Causes:**
- VGA initialization failed
- Kernel not loaded by GRUB
- Triple fault (CPU reset loop)

**Debug Steps:**
1. Check QEMU serial output: `qemu-system-i386 -serial stdio`
2. Enable QEMU debug: `qemu-system-i386 -d int,cpu_reset`
3. Verify Multiboot header is valid

### Kernel Panic

**Symptom:** "PANIC" message or halt

**Possible Causes:**
- Page fault during initialization
- Failed memory allocation
- Invalid memory access

**Debug Steps:**
1. Check error code in panic message
2. Use GDB: `qemu-system-i386 -s -S`, then `gdb kernel.elf`
3. Check CR2 register for page fault address

### Infinite Loop

**Symptom:** System hangs, no progress

**Possible Causes:**
- Deadlock in initialization
- Infinite loop in interrupt handler
- Failed interrupt handling

**Debug Steps:**
1. Use QEMU monitor: Ctrl+Alt+2
2. Check CPU state: `info registers`
3. Single-step with GDB

## Next Steps

After successful boot, the kernel:
1. Creates the idle process
2. Waits for timer interrupts
3. Ready to load user processes (Phase 3)

For more information, see:
- [MEMORY_MAP.md](MEMORY_MAP.md) - Detailed memory layout
- [INTERRUPTS.md](INTERRUPTS.md) - Interrupt handling details
- [DEVELOPMENT.md](DEVELOPMENT.md) - Build and debug instructions
