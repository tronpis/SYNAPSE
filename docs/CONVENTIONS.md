# SYNAPSE SO - Calling Conventions and Stack Layout

## Calling Conventions

### C Calling Convention (cdecl)

SYNAPSE SO uses the standard **cdecl** calling convention for C functions.

#### Function Call

**Caller responsibilities:**
1. Push arguments onto stack (right-to-left)
2. Call function (`call` instruction)
3. Clean up stack after return (`add esp, N`)

**Callee responsibilities:**
1. Save EBP (`push ebp`)
2. Set up stack frame (`mov ebp, esp`)
3. Allocate local variables (`sub esp, N`)
4. Execute function body
5. Place return value in EAX
6. Restore stack frame (`mov esp, ebp`)
7. Restore EBP (`pop ebp`)
8. Return (`ret`)

#### Example

```c
int add(int a, int b) {
    return a + b;
}

int result = add(3, 5);
```

**Assembly (caller):**
```asm
push 5          ; Second argument
push 3          ; First argument
call add        ; Call function
add esp, 8      ; Clean up (2 args × 4 bytes)
; EAX now contains return value (8)
```

**Assembly (callee):**
```asm
add:
    push ebp            ; Save old base pointer
    mov ebp, esp        ; Set up new stack frame
    
    mov eax, [ebp+8]    ; Get first arg (a)
    add eax, [ebp+12]   ; Add second arg (b)
    
    pop ebp             ; Restore base pointer
    ret                 ; Return (EAX = result)
```

#### Stack Layout During Function Call

```
Before call:                After call (in callee):
Higher addresses            Higher addresses
┌──────────────┐            ┌──────────────┐
│              │            │ Caller's     │
│ Caller's     │            │ stack frame  │
│ stack frame  │            ├──────────────┤
├──────────────┤            │ Argument 2   │ ← [EBP+12]
│              │            ├──────────────┤
│              │            │ Argument 1   │ ← [EBP+8]
│              │            ├──────────────┤
│              │            │ Return addr  │ ← [EBP+4]
│              │            ├──────────────┤
└──────────────┘ ← ESP      │ Saved EBP    │ ← [EBP], EBP points here
                            ├──────────────┤
                            │ Local vars   │
                            └──────────────┘ ← ESP
                            Lower addresses
```

### Register Usage

#### Caller-Saved (Volatile)
These registers may be modified by the callee:
- **EAX:** Return value, scratch register
- **ECX:** Scratch register
- **EDX:** Scratch register

**Note:** Caller must save these if needed after call.

#### Callee-Saved (Non-Volatile)
These registers must be preserved by callee:
- **EBX:** Base register
- **ESI:** Source index
- **EDI:** Destination index
- **EBP:** Base pointer (stack frame)

**Note:** Callee must push/pop these if used.

#### Special Registers
- **ESP:** Stack pointer (always points to top of stack)
- **EIP:** Instruction pointer (program counter)
- **EFLAGS:** CPU flags (comparison results, etc.)

### System V ABI (x86-32)

While we primarily use cdecl, some aspects of System V ABI apply:

- **Return values:**
  - 32-bit or smaller: EAX
  - 64-bit: EDX:EAX (high:low)
  - Structures: Pointer returned in EAX

- **Stack alignment:**
  - ESP should be 16-byte aligned before `call`
  - Improves performance with SSE instructions

## Interrupt Handling

### Interrupt Stack Frame

When an interrupt occurs, the CPU automatically pushes:

```
Higher addresses
┌──────────────┐
│ Old SS       │ ← Only for privilege change
├──────────────┤
│ Old ESP      │ ← Only for privilege change
├──────────────┤
│ EFLAGS       │
├──────────────┤
│ CS           │
├──────────────┤
│ EIP          │ ← ESP points here after interrupt
└──────────────┘
Lower addresses
```

**For some exceptions, CPU also pushes error code:**
```
┌──────────────┐
│ Error Code   │
├──────────────┤
│ EIP          │
├──────────────┤
│ CS           │
├──────────────┤
│ EFLAGS       │
└──────────────┘
```

### Exception/Interrupt Types

#### With Error Code
- #8: Double Fault
- #10: Invalid TSS
- #11: Segment Not Present
- #12: Stack-Segment Fault
- #13: General Protection Fault
- #14: Page Fault
- #17: Alignment Check
- #21: Control Protection Exception
- #30: Security Exception

#### Without Error Code
All other exceptions and hardware interrupts.

### Assembly Interrupt Stub

Our ISR stubs in `kernel/isr.asm`:

**For interrupts without error code:**
```asm
isr0:
    push 0              ; Dummy error code (for consistency)
    push 0              ; Interrupt number
    jmp isr_common
```

**For interrupts with error code:**
```asm
isr8:
    ; CPU already pushed error code
    push 8              ; Interrupt number
    jmp isr_common
```

**Common handler:**
```asm
isr_common:
    pusha               ; Push all general registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    
    push ds             ; Push data segment
    push es
    push fs
    push gs
    
    mov ax, 0x10        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Push pointer to register structure
    call idt_handler    ; Call C handler
    add esp, 4          ; Clean up
    
    pop gs              ; Restore segments
    pop fs
    pop es
    pop ds
    
    popa                ; Restore general registers
    add esp, 8          ; Clean up error code and int number
    iret                ; Return from interrupt
```

### Register Structure

The `registers_t` structure passed to C handlers:

```c
typedef struct {
    /* Segments */
    uint32_t gs, fs, es, ds;
    
    /* General purpose registers (pusha) */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    
    /* Interrupt information */
    uint32_t int_no, err_code;
    
    /* CPU-pushed registers */
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;
```

**Stack layout when C handler is called:**
```
Higher addresses
┌──────────────┐
│ SS           │ ← regs.ss
├──────────────┤
│ User ESP     │ ← regs.useresp
├──────────────┤
│ EFLAGS       │ ← regs.eflags
├──────────────┤
│ CS           │ ← regs.cs
├──────────────┤
│ EIP          │ ← regs.eip
├──────────────┤
│ Error Code   │ ← regs.err_code
├──────────────┤
│ Int Number   │ ← regs.int_no
├──────────────┤
│ EAX          │ ← regs.eax
├──────────────┤
│ ECX          │
├──────────────┤
│ ...          │
├──────────────┤
│ GS           │ ← regs.gs
├──────────────┤
│ &regs        │ ← Pushed by stub
└──────────────┘ ← ESP in C handler
Lower addresses
```

### Interrupt Return (IRET)

The `iret` instruction:
1. Pops EIP, CS, EFLAGS
2. If privilege level changed, pops ESP and SS
3. Resumes execution at EIP

**Note:** Interrupts are re-enabled unless IF flag is cleared.

## System Calls

### Linux-Style System Call Interface

SYNAPSE SO uses interrupt `0x80` for system calls (like Linux).

#### Making a System Call

**From user space:**
```asm
mov eax, syscall_number    ; System call number
mov ebx, arg1              ; First argument
mov ecx, arg2              ; Second argument
mov edx, arg3              ; Third argument
mov esi, arg4              ; Fourth argument
mov edi, arg5              ; Fifth argument
int 0x80                   ; Invoke system call
; Return value in EAX
```

**Example: sys_write**
```asm
mov eax, 1        ; SYS_WRITE = 1
mov ebx, 1        ; fd = 1 (stdout)
mov ecx, buffer   ; buffer pointer
mov edx, 13       ; count = 13
int 0x80          ; syscall
```

#### System Call Numbers

```c
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_GETPID  8
```

#### System Call Handler

In `kernel/syscall.c`:

```c
void syscall_handler(registers_t* regs) {
    uint32_t num = regs->eax;           // Syscall number
    uint32_t arg1 = regs->ebx;          // Arguments
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    uint32_t arg5 = regs->edi;
    
    // Validate and dispatch
    if (num < NUM_SYSCALLS && syscall_table[num]) {
        uint32_t ret = syscall_table[num](arg1, arg2, arg3, arg4, arg5);
        regs->eax = ret;                // Set return value
    } else {
        regs->eax = -1;                 // Error
    }
}
```

**Key Points:**
- System call number in EAX
- Up to 5 arguments in EBX, ECX, EDX, ESI, EDI
- Return value in EAX
- C function signature: `int syscall(uint32_t arg1, ...)`

### User Pointer Validation

**CRITICAL:** System calls must validate user pointers!

**Bad (unsafe):**
```c
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count) {
    char* buf = (char*)buffer;  // DANGEROUS! No validation
    for (uint32_t i = 0; i < count; i++) {
        vga_put_char(buf[i]);   // May access invalid memory
    }
    return count;
}
```

**Good (safe with temporary mapping):**
```c
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count) {
    uint32_t bytes_written = 0;
    
    while (bytes_written < count) {
        // Get physical address (validates mapping)
        uint32_t phys = vmm_get_phys_addr(buffer & 0xFFFFF000);
        if (phys == 0) return -1;  // Invalid pointer
        
        // Map to kernel temporarily
        int slot = vmm_alloc_temp_slot();
        uint32_t temp = vmm_map_temp_page(phys, slot);
        
        // Access safely
        char* buf = (char*)(temp + (buffer & 0xFFF));
        vga_put_char(*buf);
        
        // Cleanup
        vmm_unmap_temp_page(slot);
        vmm_free_temp_slot(slot);
        
        bytes_written++;
        buffer++;
    }
    
    return bytes_written;
}
```

## Context Switching

### Process Context

Each process has saved context in PCB:

```c
typedef struct {
    /* CPU registers */
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    
    /* Segment registers */
    uint32_t cs, ds, es, fs, gs, ss;
    
    /* Page directory */
    page_directory_t* page_dir;
    
    /* ... other fields ... */
} process_t;
```

### Context Switch Assembly

In `kernel/switch.asm`:

```asm
switch_context:
    ; Save current context
    push ebp
    push ebx
    push esi
    push edi
    
    ; Save ESP to old process
    mov eax, [esp+20]   ; old process
    mov [eax], esp      ; Save ESP
    
    ; Switch page directory
    mov eax, [esp+24]   ; new process
    mov ecx, [eax+4]    ; Get page_dir field
    mov cr3, ecx        ; Switch address space
    
    ; Load new ESP
    mov esp, [eax]      ; Load new ESP
    
    ; Restore new context
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret
```

**Key Points:**
- Saves/restores callee-saved registers only
- Switches page directory (CR3)
- Returns to new process's saved EIP

### Initial Context Setup

For new processes:

```c
void process_setup_initial_context(process_t* proc, uint32_t entry_point) {
    /* Setup stack with fake interrupt frame */
    uint32_t* stack = (uint32_t*)(proc->kernel_stack + KERNEL_STACK_SIZE);
    
    stack--;
    *stack = 0x23;              // SS (user data segment)
    stack--;
    *stack = proc->user_stack;  // ESP
    stack--;
    *stack = 0x202;             // EFLAGS (IF=1)
    stack--;
    *stack = 0x1B;              // CS (user code segment)
    stack--;
    *stack = entry_point;       // EIP
    
    /* Setup for iret */
    proc->esp = (uint32_t)stack;
}
```

## Stack Layout Summary

### Kernel Stack

Each process has a kernel stack (8KB):

```
Higher addresses (stack top)
┌──────────────────┐
│ Guard page (opt) │
├──────────────────┤
│ Available space  │
│       ...        │
│       ...        │
├──────────────────┤
│ Current frame    │
│ - Local vars     │
│ - Saved regs     │
│ - Return addr    │
└──────────────────┘ ← ESP
Lower addresses
```

### User Stack

User processes have separate user stack:

```
Higher addresses
┌──────────────────┐ ← Stack base (e.g., 0x40000000)
│ Environment vars │
├──────────────────┤
│ Command args     │
├──────────────────┤
│ Available space  │
│       ...        │
├──────────────────┤
│ Current frame    │
└──────────────────┘ ← User ESP
Lower addresses
```

## Best Practices

### For Kernel Code

1. **Always validate user pointers** before dereferencing
2. **Use temporary mappings** to access user memory
3. **Check return values** from allocation functions
4. **Disable interrupts** for critical sections
5. **Keep interrupt handlers short** and fast
6. **Use inline assembly sparingly** and document it

### For System Calls

1. **Validate all parameters** from user space
2. **Check buffer bounds** before copying
3. **Return error codes** consistently (-1 for error)
4. **Don't trust user data** - always validate
5. **Use temporary mappings** for user buffers

### For Assembly Code

1. **Follow calling convention** (cdecl)
2. **Preserve callee-saved registers**
3. **Align stack** to 16 bytes before calls
4. **Document register usage** in comments
5. **Add .note.GNU-stack** section to prevent warnings

## References

- [System V ABI](https://wiki.osdev.org/System_V_ABI)
- [Calling Conventions](https://en.wikipedia.org/wiki/X86_calling_conventions)
- [Intel Manual: Interrupt Handling](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.html)
