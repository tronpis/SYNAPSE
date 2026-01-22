# SYNAPSE SO - Testing Guide

## Overview

This document describes the testing strategy and available tests for SYNAPSE SO.

## Test Categories

### 1. Boot Tests

Verify that the kernel boots successfully.

**Test:** Kernel initialization
**Expected output:**
```
[+] VGA initialized
[+] GDT initialized
[+] IDT initialized
[+] Physical Memory Manager initialized
[+] Virtual Memory Manager initialized
[+] Heap initialized
[+] Timer initialized
[+] System Call Interface initialized
[+] Process Manager initialized
[+] Scheduler initialized
[+] Kernel initialized successfully
```

**How to run:**
```bash
./build.sh test
```

### 2. Memory Tests

Verify memory management functions correctly.

#### PMM Tests
- Frame allocation returns non-zero addresses
- Frame deallocation works correctly
- Memory statistics are accurate
- Out-of-memory handling

#### VMM Tests
- Page mapping works
- Page unmapping works
- Temporary mappings don't leak frames
- Page fault detection
- Address space isolation

#### Heap Tests
- kmalloc returns valid pointers
- kfree doesn't crash
- krealloc works correctly
- Coalescing works
- Large allocations work

### 3. System Call Tests

Verify system calls work from user mode (Phase 3+).

**sys_write test:**
```c
char* message = "Hello from user space\n";
int ret = syscall(SYS_WRITE, 1, message, strlen(message));
assert(ret == strlen(message));
```

**sys_getpid test:**
```c
int pid = syscall(SYS_GETPID);
assert(pid > 0);
```

### 4. Interrupt Tests

Verify interrupt handling works correctly.

**Timer test:**
- Timer IRQ fires regularly
- Tick counter increments
- Scheduler tick is called

**Page fault test:**
- Invalid read triggers page fault
- Invalid write triggers page fault
- Fault address is correct

## Running Tests

### Quick Test (Boot Only)

```bash
./build.sh test
```

This runs the kernel in QEMU with a 10-second timeout and checks for successful boot.

### Manual Testing

```bash
./build.sh run
```

This runs the kernel in QEMU with full display. Observe the output for error messages.

### Debug Testing

```bash
./build.sh debug
```

This starts QEMU with GDB server on port 1234. In another terminal:

```bash
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Test Infrastructure

### Assertions

**Simple assertion macro:**
```c
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            vga_print("[-] ASSERTION FAILED: "); \
            vga_print(message); \
            vga_print("\n    at "); \
            vga_print(__FILE__); \
            vga_print(":"); \
            vga_print_dec(__LINE__); \
            vga_print("\n"); \
            __asm__ volatile("cli; hlt"); \
        } \
    } while(0)
```

**Usage:**
```c
ASSERT(ptr != 0, "Memory allocation failed");
ASSERT(result == expected, "Calculation incorrect");
```

### Test Results

**Success indicators:**
- `[+]` prefix for successful operations
- `[*]` prefix for informational messages
- No panic or halt

**Failure indicators:**
- `[-]` prefix for errors
- `PANIC` message
- System halt (HLT instruction)
- Triple fault (QEMU reset)

## Example Test Cases

### Test 1: PMM Allocation

```c
void test_pmm_allocation(void) {
    vga_print("[TEST] PMM allocation\n");
    
    /* Allocate 10 frames */
    uint32_t frames[10];
    for (int i = 0; i < 10; i++) {
        frames[i] = pmm_alloc_frame();
        ASSERT(frames[i] != 0, "Frame allocation failed");
        ASSERT(frames[i] % PAGE_SIZE == 0, "Frame not aligned");
    }
    
    /* Free all frames */
    for (int i = 0; i < 10; i++) {
        pmm_free_frame(frames[i]);
    }
    
    vga_print("    PASSED\n");
}
```

### Test 2: VMM Mapping

```c
void test_vmm_mapping(void) {
    vga_print("[TEST] VMM page mapping\n");
    
    /* Allocate a frame */
    uint32_t phys = pmm_alloc_frame();
    ASSERT(phys != 0, "Frame allocation failed");
    
    /* Map to virtual address */
    uint32_t virt = 0x10000000;  /* 256MB */
    vmm_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE);
    
    /* Verify mapping */
    uint32_t phys_check = vmm_get_phys_addr(virt);
    ASSERT(phys_check == phys, "Mapping incorrect");
    
    /* Write to page */
    uint32_t* ptr = (uint32_t*)virt;
    *ptr = 0xDEADBEEF;
    ASSERT(*ptr == 0xDEADBEEF, "Write failed");
    
    /* Unmap */
    vmm_unmap_page(virt);
    
    vga_print("    PASSED\n");
}
```

### Test 3: Heap Allocation

```c
void test_heap_allocation(void) {
    vga_print("[TEST] Heap allocation\n");
    
    /* Allocate various sizes */
    void* ptr1 = kmalloc(64);
    ASSERT(ptr1 != 0, "kmalloc 64 failed");
    
    void* ptr2 = kmalloc(1024);
    ASSERT(ptr2 != 0, "kmalloc 1024 failed");
    
    void* ptr3 = kmalloc(4096);
    ASSERT(ptr3 != 0, "kmalloc 4096 failed");
    
    /* Verify pointers are different */
    ASSERT(ptr1 != ptr2, "Overlapping allocations");
    ASSERT(ptr2 != ptr3, "Overlapping allocations");
    
    /* Free in different order */
    kfree(ptr2);
    kfree(ptr1);
    kfree(ptr3);
    
    vga_print("    PASSED\n");
}
```

### Test 4: Temporary Mapping

```c
void test_temp_mapping(void) {
    vga_print("[TEST] Temporary mapping\n");
    
    /* Allocate a frame */
    uint32_t phys = pmm_alloc_frame();
    ASSERT(phys != 0, "Frame allocation failed");
    
    /* Write to frame using temporary mapping */
    int slot = vmm_alloc_temp_slot();
    ASSERT(slot >= 0, "Temp slot allocation failed");
    
    uint32_t virt = vmm_map_temp_page(phys, slot);
    ASSERT(virt != 0, "Temp mapping failed");
    
    /* Write test pattern */
    uint32_t* ptr = (uint32_t*)virt;
    *ptr = 0x12345678;
    
    /* Unmap */
    vmm_unmap_temp_page(slot);
    vmm_free_temp_slot(slot);
    
    /* Verify frame not freed */
    /* (We'd need to implement a frame usage check for this) */
    
    /* Manually free the frame */
    pmm_free_frame(phys);
    
    vga_print("    PASSED\n");
}
```

### Test 5: System Call (User Mode)

**Note:** Requires user mode process (Phase 3)

```c
void test_syscall_write(void) {
    vga_print("[TEST] System call: write\n");
    
    char* message = "Test message\n";
    int ret = sys_write(1, (uint32_t)message, 13);
    
    ASSERT(ret == 13, "sys_write returned wrong count");
    
    vga_print("    PASSED\n");
}
```

## Automated Test Script

Create a test runner in kernel:

```c
void run_tests(void) {
    vga_print("\n");
    vga_print("===================================\n");
    vga_print("   SYNAPSE SO TEST SUITE\n");
    vga_print("===================================\n");
    vga_print("\n");
    
    test_pmm_allocation();
    test_vmm_mapping();
    test_heap_allocation();
    test_temp_mapping();
    
    vga_print("\n");
    vga_print("===================================\n");
    vga_print("   ALL TESTS PASSED\n");
    vga_print("===================================\n");
    vga_print("\n");
}
```

Call from `kernel_main()`:

```c
#ifdef RUN_TESTS
    run_tests();
#endif
```

Build with tests:
```bash
make CFLAGS="-DRUN_TESTS" test
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y gcc gcc-multilib nasm binutils \
                                grub-pc-bin xorriso qemu-system-x86
    
    - name: Build kernel
      run: ./build.sh build
    
    - name: Run tests
      run: ./build.sh test
    
    - name: Upload ISO
      uses: actions/upload-artifact@v2
      with:
        name: synapse.iso
        path: synapse.iso
```

## Test Coverage Goals

### Phase 2 (Current)
- [x] Boot successfully
- [x] PMM allocates frames
- [x] VMM maps pages
- [x] Heap allocation works
- [x] Interrupts fire correctly
- [ ] Temporary mappings don't leak
- [ ] System calls work (kernel mode)

### Phase 3 (Future)
- [ ] User mode processes run
- [ ] System calls work (user mode)
- [ ] Context switching works
- [ ] Scheduler switches processes
- [ ] Process isolation verified

### Phase 4+ (Future)
- [ ] File system operations
- [ ] Network stack
- [ ] Driver loading
- [ ] Performance benchmarks

## Debugging Failed Tests

### Triple Fault

**Symptom:** QEMU resets immediately

**Possible causes:**
- Invalid page table
- Stack overflow
- Interrupt handler bug

**Debug:**
```bash
qemu-system-i386 -cdrom synapse.iso -d int,cpu_reset -no-reboot
```

Look for:
- Exception before reset
- Invalid CR3 value
- Stack pointer out of range

### Page Fault

**Symptom:** Page fault error message

**Debug:**
1. Note the fault address (CR2)
2. Note the error code
3. Check if address is valid
4. Check page table entries

**Error code bits:**
- Bit 0: 0 = not present, 1 = protection violation
- Bit 1: 0 = read, 1 = write
- Bit 2: 0 = kernel, 1 = user
- Bit 3: 1 = reserved bit violation
- Bit 4: 1 = instruction fetch

### Hang

**Symptom:** System stops responding

**Possible causes:**
- Infinite loop
- Deadlock
- Interrupts disabled
- Waiting for hardware

**Debug:**
1. Use QEMU monitor (Ctrl+Alt+2)
2. Check current EIP: `info registers`
3. Examine code at EIP
4. Check if interrupts are enabled (EFLAGS.IF)

## Performance Testing

### Timer Accuracy

```c
void test_timer_accuracy(void) {
    uint32_t start = timer_get_ticks();
    
    /* Wait for 100 ticks (1 second at 100Hz) */
    while (timer_get_ticks() - start < 100) {
        __asm__ volatile("hlt");
    }
    
    uint32_t end = timer_get_ticks();
    ASSERT(end - start >= 100 && end - start <= 105, 
           "Timer not accurate");
}
```

### Allocation Speed

```c
void test_allocation_speed(void) {
    uint32_t start = timer_get_ticks();
    
    /* Allocate and free 1000 blocks */
    for (int i = 0; i < 1000; i++) {
        void* ptr = kmalloc(64);
        kfree(ptr);
    }
    
    uint32_t end = timer_get_ticks();
    uint32_t duration = end - start;
    
    vga_print("    1000 alloc/free in ");
    vga_print_dec(duration);
    vga_print(" ticks\n");
}
```

## References

- [OSDev Testing](https://wiki.osdev.org/Testing)
- [Kernel Debugging](https://wiki.osdev.org/Kernel_Debugging)
