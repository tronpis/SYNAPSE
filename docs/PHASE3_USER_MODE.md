# SYNAPSE SO - Phase 3: User Mode Implementation

## Overview

Phase 3 implements user mode processes, completing the privilege separation between kernel and user space. This allows the OS to run untrusted code safely in ring 3.

**Date:** January 2025  
**Status:** IMPLEMENTED  
**Severity:** HIGH Priority

---

## Features Implemented

### 1. User Mode Transition

**File:** `kernel/usermode.c`, `kernel/include/kernel/usermode.h`

#### enter_usermode()

Transitions from kernel mode (ring 0) to user mode (ring 3):

```c
void enter_usermode(uint32_t entry_point, uint32_t user_stack);
```

**Implementation:**
1. Sets up user data segments (DS, ES, FS, GS)
2. Pushes interrupt return frame:
   - SS (user data segment)
   - ESP (user stack pointer)
   - EFLAGS (with IF=1 for interrupts)
   - CS (user code segment)
   - EIP (entry point)
3. Executes `iret` to switch to ring 3

**Stack Frame for iret:**
```
[Higher addresses]
+------------------+
| SS (0x20)        | ← User data segment
+------------------+
| ESP (user stack) | ← User stack pointer
+------------------+
| EFLAGS (0x202)   | ← Flags with IF set
+------------------+
| CS (0x18)        | ← User code segment
+------------------+
| EIP (entry)      | ← Entry point
+------------------+ ← ESP before iret
[Lower addresses]
```

### 2. User Process Creation

#### create_user_test_process()

Creates a test user mode process with embedded code:

**Steps:**
1. Create process structure
2. Mark as `PROC_FLAG_USER`
3. Allocate user memory:
   - Code at `0x400000` (4MB)
   - Stack at `0x800000` (8MB)
4. Map pages with `PAGE_USER` flag
5. Copy code to user space using temporary mappings
6. Set up initial state (EIP, ESP)
7. Add to scheduler

**Memory Layout:**
```
User Address Space:
0x00000000 - 0x003FFFFF  ← NULL guard + low memory (not mapped)
0x00400000 - 0x004FFFFF  ← User code (executable, readable)
0x00500000 - 0x007FFFFF  ← Heap (not allocated yet)
0x00800000 - 0x00803FFF  ← User stack (16KB, grows down)
0x00804000 - 0x3FFFFFFF  ← Available for expansion
0xC0000000 - 0xFFFFFFFF  ← Kernel space (not accessible from user mode)
```

### 3. Test User Code

The test process demonstrates:
- System calls from user mode
- Proper privilege separation
- Cooperative multitasking

**Test code functionality:**
```c
1. Write message using sys_write(1, buffer, len)
2. Get PID using sys_getpid()
3. Infinite loop with voluntary yield (int $0x20)
```

### 4. System Call Interface

System calls work from user mode:

**User Mode Syscall:**
```asm
mov eax, syscall_number    ; Syscall number
mov ebx, arg1              ; First argument
mov ecx, arg2              ; Second argument
mov edx, arg3              ; Third argument
int 0x80                   ; Invoke syscall
; Return value in EAX
```

**Flow:**
1. User code executes `int $0x80`
2. CPU switches to kernel mode (ring 0)
3. IDT entry 128 invokes syscall handler
4. Syscall handler validates arguments
5. Executes kernel function
6. Returns to user mode with result in EAX

---

## Security Model

### Privilege Separation

**Ring 0 (Kernel Mode):**
- ✅ Can access all memory
- ✅ Can execute privileged instructions
- ✅ Can modify GDT, IDT, CR3
- ✅ Can access I/O ports

**Ring 3 (User Mode):**
- ❌ Cannot access kernel memory (page fault)
- ❌ Cannot execute privileged instructions (general protection fault)
- ❌ Cannot access I/O ports directly
- ✅ Can only access pages with `PAGE_USER` flag
- ✅ Can invoke system calls via `int $0x80`

### Memory Protection

**Page Flags:**
- `PAGE_PRESENT | PAGE_WRITE | PAGE_USER` - User read/write
- `PAGE_PRESENT | PAGE_USER` - User read-only
- `PAGE_PRESENT | PAGE_WRITE` - Kernel only

**Protection Tests:**
```c
/* From user mode */
char* kernel_addr = (char*)0xC0000000;
char c = *kernel_addr;  // PAGE FAULT (no PAGE_USER)

/* Privileged instruction */
__asm__ volatile("cli");  // GENERAL PROTECTION FAULT
```

### System Call Validation

All system calls must:
1. **Validate user pointers** before dereferencing
2. **Check permissions** for requested operations
3. **Limit resource usage** (DOS prevention)
4. **Handle errors gracefully** (no kernel panic)

**Example: sys_write validation:**
```c
1. Check count is reasonable (max 4096)
2. Reject kernel-space pointers (addr >= 0xC0000000)
3. Verify each page is mapped (vmm_get_phys_addr)
4. Use temporary mappings to access user memory
5. Return partial success on error
```

---

## Scheduler Integration

### Process States

User processes go through the same states as kernel processes:

```
READY → RUNNING → READY → ...
  ↓
BLOCKED (waiting for I/O)
  ↓
READY
  ↓
ZOMBIE (exited, waiting for parent)
```

### Context Switching

When switching to/from user processes:

1. **Save context** (registers, EIP, ESP)
2. **Switch page directory** (CR3)
3. **Load new context**
4. **Return via iret** (automatically switches privilege level)

**Important:** The scheduler doesn't care about privilege level - it just restores the saved context, and `iret` handles the ring transition.

### Time Quantum

User processes use the same quantum as kernel processes (default 10 ticks @ 100Hz = 100ms).

---

## Testing

### Test 1: User Mode Transition

```c
/* Kernel creates user process */
uint32_t pid = create_user_test_process();
// Expected: Process created, executes in user mode
```

**Verification:**
- Process structure created with `PROC_FLAG_USER`
- Code mapped at 0x400000 with PAGE_USER
- Process added to scheduler
- Runs without page faults

### Test 2: System Call from User Mode

```c
/* User mode code */
__asm__ volatile(
    "mov $1, %eax\n"      // SYS_WRITE
    "mov $1, %ebx\n"      // fd = 1
    "mov %0, %ecx\n"      // buffer
    "mov %1, %edx\n"      // count
    "int $0x80\n"         // syscall
    : : "r"(msg), "r"(len)
);
// Expected: Message printed, no crash
```

**Verification:**
- Syscall handler invoked
- User pointer validated
- Message printed
- Return to user mode

### Test 3: Memory Protection

```c
/* User mode tries to access kernel memory */
char* kernel = (char*)0xC0000000;
char c = *kernel;
// Expected: PAGE FAULT
```

**Verification:**
- Page fault handler invoked
- Error code shows user mode access
- System handles fault gracefully

### Test 4: Privileged Instruction

```c
/* User mode tries privileged instruction */
__asm__ volatile("cli");
// Expected: GENERAL PROTECTION FAULT
```

**Verification:**
- Exception #13 (GPF) triggered
- System handles fault gracefully

### Test 5: Multi-Process Scheduling

```c
/* Create multiple user processes */
create_user_test_process();
create_user_test_process();
// Expected: Both run, scheduler switches between them
```

**Verification:**
- Both processes execute
- Scheduler switches correctly
- No conflicts in user memory

---

## Performance

### Context Switch Overhead

**User → Kernel (syscall):**
- Save user registers: ~20 cycles
- Switch to kernel stack: ~10 cycles
- Validate arguments: ~50 cycles
- Execute syscall: varies
- **Total:** ~100-1000 cycles

**Kernel → User (return):**
- Restore user registers: ~20 cycles
- iret with ring change: ~30 cycles
- **Total:** ~50 cycles

**Process Switch (user ↔ user):**
- Save context: ~50 cycles
- Switch page directory: ~50 cycles
- TLB flush (implicit in CR3 write): ~100 cycles
- Load context: ~50 cycles
- **Total:** ~250 cycles

### Optimization Opportunities

1. **TLB caching** - Keep frequently accessed mappings
2. **Global pages** - Mark kernel pages as global to avoid TLB flush
3. **Fast path syscalls** - Optimize common syscalls (sysenter/sysexit in future)
4. **Process-local temporary mappings** - Reduce contention

---

## Known Limitations

### Phase 3 Limitations

1. **No fork() implementation** - Requires copy-on-write
2. **No exec() implementation** - Requires ELF loading from disk
3. **No signals** - Process cannot be interrupted
4. **No IPC** - Processes can't communicate
5. **No file system** - Can't load external programs
6. **Single core only** - No SMP support yet

### Future Work (Phase 4+)

1. **Dynamic loading** - Load ELF binaries from disk
2. **Fork/exec** - Full process creation
3. **Signals** - Interrupt processes for events
4. **IPC** - Pipes, shared memory, message passing
5. **Virtual filesystem** - Abstract file operations
6. **SMP support** - Multi-core scheduling

---

## Code Organization

### New Files

```
kernel/
├── usermode.c                    # User mode transition implementation
├── include/kernel/
│   └── usermode.h                # User mode API
└── kernel.c                      # Updated to create user process
```

### Modified Files

```
kernel/
├── kernel.c                      # Added user process creation
└── Makefile                      # Added usermode.c to build
```

### API Summary

```c
/* User mode API */
void enter_usermode(uint32_t entry_point, uint32_t user_stack);
uint32_t create_user_test_process(void);

/* System calls available from user mode */
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count);
int sys_read(uint32_t fd, uint32_t buffer, uint32_t count);
int sys_exit(uint32_t exit_code);
int sys_getpid(void);
/* ... more syscalls ... */
```

---

## Debugging User Mode

### Common Issues

**Problem:** User process page faults immediately

**Solution:**
- Verify code mapped with `PAGE_USER` flag
- Check entry point is correct
- Verify stack is mapped and valid

**Problem:** Syscall returns -1

**Solution:**
- Check user pointer validation
- Verify page is mapped in user space
- Check syscall number is valid

**Problem:** General protection fault

**Solution:**
- User code tries privileged instruction
- Invalid segment selector
- Stack overflow (ESP out of range)

### Debug Commands

```c
/* Print process info */
vga_print("Process: ");
vga_print_dec(proc->pid);
vga_print(" at EIP=");
vga_print_hex(proc->eip);
vga_print(" ESP=");
vga_print_hex(proc->esp);

/* Check page mapping */
uint32_t phys = vmm_get_phys_addr(user_addr);
if (phys == 0) {
    vga_print("Page not mapped!");
}
```

---

## References

- [Intel® 64 and IA-32 Architectures Software Developer's Manual, Vol. 3A](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.html) - Privilege levels, protection
- [OSDev Wiki: Getting to Ring 3](https://wiki.osdev.org/Getting_to_Ring_3)
- [Linux System Call Interface](https://man7.org/linux/man-pages/man2/syscalls.2.html)
- [CONVENTIONS.md](CONVENTIONS.md) - System call conventions
- [MEMORY_MAP.md](MEMORY_MAP.md) - Memory layout

---

**Status:** ✅ IMPLEMENTED  
**Last Updated:** January 2025  
**Next Phase:** File System & Dynamic Loading
