/* SYNAPSE SO - User Mode Transition */
/* Licensed under GPLv3 */

#include <kernel/usermode.h>
#include <kernel/process.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <kernel/gdt.h>

/* GDT segment selectors from gdt.h */
#define USER_CODE_SELECTOR  0x18  /* User code segment (GDT entry 3) */
#define USER_DATA_SELECTOR  0x20  /* User data segment (GDT entry 4) */

/* Enter user mode and jump to entry point
 * Sets up user mode stack and segments, then uses iret to switch to ring 3
 */
void enter_usermode(uint32_t entry_point, uint32_t user_stack) {
    /* Set up user data segments */
    __asm__ volatile(
        "mov $0x20, %%ax\n"     /* USER_DATA_SELECTOR = 0x20 */
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        ::: "%eax"
    );
    
    /* Push user mode stack frame for iret
     * Stack layout (pushed in reverse order):
     * SS      ← Stack segment (user data)
     * ESP     ← User stack pointer
     * EFLAGS  ← Flags (with IF=1 for interrupts)
     * CS      ← Code segment (user code)
     * EIP     ← Entry point
     */
    __asm__ volatile(
        "pushl $0x20\n"         /* SS: user data segment */
        "pushl %0\n"            /* ESP: user stack */
        "pushf\n"               /* EFLAGS */
        "pop %%eax\n"
        "or $0x200, %%eax\n"    /* Set IF (enable interrupts) */
        "push %%eax\n"
        "pushl $0x18\n"         /* CS: user code segment */
        "pushl %1\n"            /* EIP: entry point */
        "iret\n"                /* Switch to user mode */
        :
        : "r"(user_stack), "r"(entry_point)
        : "%eax"
    );
    
    /* Should never reach here */
    __builtin_unreachable();
}

/* Simple user mode test code
 * This code will be copied to user space and executed
 */
static void user_test_code(void) {
    /* This function runs in user mode (ring 3) */
    
    /* Test syscall: write a message */
    const char* message = "Hello from user mode!\n";
    uint32_t len = 0;
    while (message[len]) len++;
    
    /* Make syscall: write(1, message, len) */
    __asm__ volatile(
        "mov $1, %%eax\n"       /* syscall number: SYS_WRITE */
        "mov $1, %%ebx\n"       /* fd = 1 (stdout) */
        "mov %0, %%ecx\n"       /* buffer */
        "mov %1, %%edx\n"       /* count */
        "int $0x80\n"           /* invoke syscall */
        :
        : "r"(message), "r"(len)
        : "%eax", "%ebx", "%ecx", "%edx"
    );
    
    /* Test getpid syscall */
    uint32_t pid;
    __asm__ volatile(
        "mov $8, %%eax\n"       /* syscall number: SYS_GETPID */
        "int $0x80\n"           /* invoke syscall */
        "mov %%eax, %0\n"       /* save result */
        : "=r"(pid)
        :
        : "%eax"
    );
    
    /* Infinite loop - user process stays alive */
    while (1) {
        /* Yield to other processes */
        __asm__ volatile("int $0x20");  /* Timer interrupt (yield) */
        
        /* Small delay */
        for (volatile int i = 0; i < 1000000; i++);
    }
}

/* Marker to find end of user code */
static void user_test_code_end(void) {
    /* This is just a marker, not executed */
}

/* Create a user mode test process */
uint32_t create_user_test_process(void) {
    vga_print("[-] User mode test process not available yet\n");
    vga_print("    (Requires a proper TSS/privilege transition implementation)\n");
    return 0;
}
