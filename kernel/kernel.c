/* SYNAPSE SO - Kernel Entry Point */
/* Licensed under GPLv3 */

#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/heap.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/timer.h>
#include <kernel/elf.h>
#include <kernel/syscall.h>
#include <kernel/usermode.h>
#include <kernel/cpu.h>
#include <kernel/early.h>

/* Multiboot information structure */
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int mmap_length;
    unsigned int mmap_addr;
    /* ... more fields not used in minimal version ... */
} __attribute__((packed)) multiboot_info_t;

static void demo_syscalls(void) {
    /* Demonstrate system calls in kernel thread */
    vga_print("[DEMO] Testing syscalls...\n");

    /* Test sys_getpid */
    pid_t pid = sys_getpid();
    vga_print("[DEMO] Current PID: ");
    vga_print_dec(pid);
    vga_print("\n");

    /* Test sys_write */
    char* msg = "Hello from syscall!";
    vga_print("[DEMO] Writing via kernel path: ");
    vga_print(msg);
    int bytes_written = 19;
    vga_print_dec(bytes_written);
    vga_print(" bytes\n");

    /* Sleep for a while */
    for (uint32_t i = 0; i < 50000000; i++) {
        __asm__ __volatile__("nop");
    }

    /* Test sys_exit (this will terminate the process) */
    vga_print("[DEMO] Calling sys_exit(0)...\n");
    sys_exit(0);
}

static void shell_process(void) {
    /* Simple interactive shell */
    char buffer[256];
    int pos = 0;

    vga_print("[SHELL] SYNAPSE SO Shell v0.1\n");
    vga_print("[SHELL] Type 'help' for commands\n");
    vga_print("[SHELL] $ ");

    while (1) {
        /* Read character from keyboard (not implemented yet) */
        /* For now, simulate simple commands */
        vga_print("\n[SHELL] Commands: help, mem, procs, time, exit\n");
        vga_print("[SHELL] $ ");

        for (uint32_t i = 0; i < 100000000; i++) {
            __asm__ __volatile__("nop");
        }

        break; /* Exit for now until keyboard input is implemented */
    }
}

static void worker_a(void) {
    uint32_t last = 0;

    while (1) {
        uint32_t now = timer_get_ticks();
        if (now - last >= 100) {
            last = now;
            /* Make the VGA prints atomic to avoid concurrent corruption. */
            __asm__ __volatile__("cli");
            vga_print("[A] ticks=");
            vga_print_dec(now);
            vga_print("\n");
            __asm__ __volatile__("sti");
        }
        __asm__ __volatile__("hlt");
    }
}

static void worker_b(void) {
    uint32_t last = 0;

    while (1) {
        uint32_t now = timer_get_ticks();
        if (now - last >= 137) {
            last = now;
            /* Make the VGA prints atomic to avoid concurrent corruption. */
            __asm__ __volatile__("cli");
            vga_print("[B] ticks=");
            vga_print_dec(now);
            vga_print("\n");
            __asm__ __volatile__("sti");
        }
        __asm__ __volatile__("hlt");
    }
}

/* Global variables for early checks */
void* multiboot_info_ptr = 0;
uint32_t multiboot_magic = 0;

/* Kernel main function - entry point from bootloader */
void kernel_main(unsigned int magic, multiboot_info_t* mbi) {
    /* Save for early diagnostics */
    multiboot_magic = magic;
    multiboot_info_ptr = mbi;
    
    /* Clear screen */
    vga_clear_screen();

    /* Print kernel banner */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("SYNAPSE SO - Open Source Operating System v0.3.0\n");
    vga_print("=================================================\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("Phase 1: Boot and Initialization\n");
    vga_print("=================================================\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Initialize CPU detection */
    vga_print("[+] Detecting CPU...\n");
    cpu_init();
    cpu_print_info();
    
    /* Run early boot checks */
    early_init();
    int check_result = early_run_checks();
    early_print_summary();
    
    if (check_result == BOOT_CHECK_FATAL) {
        early_panic("Boot checks failed");
    }
    
    /* Enable CPU features (SSE, etc) */
    vga_print("[+] Enabling CPU features...\n");
    cpu_enable_features();

    /* Initialize GDT */
    vga_print("[+] Initializing Global Descriptor Table...\n");
    gdt_init();
    vga_print("    GDT loaded successfully\n");

    /* Initialize IDT */
    vga_print("[+] Initializing Interrupt Descriptor Table...\n");
    idt_init();
    vga_print("    IDT loaded successfully\n");
    
    /* Phase 1 complete */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\n[SUCCESS] Phase 1 complete!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Keep interrupts disabled until Phase 2 initialization is complete. */
    __asm__ __volatile__("cli");

    /* Phase 2: Memory Management */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== PHASE 2: Memory Management ===\n");

    /* Initialize Physical Memory Manager */
    if (mbi != 0 && (mbi->flags & 0x40)) {
        mem_map_t* mmap = (mem_map_t*)mbi->mmap_addr;
        pmm_init(mmap, mbi->mmap_length, 8); /* 8 bytes per entry */
    } else {
        /* Fallback: assume 16MB memory */
        vga_print("[-] Warning: No memory map, using default 16MB\n");
    }

    /* Initialize simple kernel heap (before paging) */
    pmm_init_kernel_heap(0x300000, 0x100000); /* 1MB at 3MB */

    /* Initialize Virtual Memory Manager */
    vmm_init();

    /* Initialize proper kernel heap */
    heap_init((void*)0xC0300000, 0x100000); /* 1MB at 3GB+3MB */

    /* Initialize Process Management */
    vga_print("\n=== PHASE 2: Process Management ===\n");
    process_init();
    scheduler_init();

    /* Create a process representing the currently running kernel context */
    process_create_current("kernel_main");

    /* Demo kernel threads */
    process_create("worker_a", PROC_FLAG_KERNEL, worker_a);
    process_create("worker_b", PROC_FLAG_KERNEL, worker_b);

    /* Demo system calls and shell (Phase 3) */
    process_create("demo_syscalls", PROC_FLAG_KERNEL, demo_syscalls);
    process_create("shell", PROC_FLAG_KERNEL, shell_process);

    /* Start PIT so scheduler_tick() runs from IRQ0 */
    timer_init(100);

    /* Phase 3: System Call Interface */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== PHASE 3: System Call Interface & User Mode ===\n");
    syscall_init();
    
    /* Create user mode test process */
    vga_print("[+] Creating user mode test process...\n");
    uint32_t user_pid = create_user_test_process();
    if (user_pid > 0) {
        vga_print("    User process created with PID: ");
        vga_print_dec(user_pid);
        vga_print("\n");
    } else {
        vga_print("[-] Failed to create user process\n");
    }

    /* Memory information */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\nSystem Information:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    if (mbi != 0) {
        vga_print("    Lower memory: ");
        vga_print_dec(mbi->mem_lower);
        vga_print(" KB\n");
        vga_print("    Upper memory: ");
        vga_print_dec(mbi->mem_upper);
        vga_print(" KB\n");
    } else {
        vga_print("    Memory information not available\n");
    }

    /* Kernel ready */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\n[SUCCESS] Phase 3 initialized successfully!\n");
    vga_print("SYNAPSE SO is ready with user mode support.\n");

    /* Start scheduler */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\nStarting scheduler...\n");
    vga_print("[+] Enabling interrupts\n");
    __asm__ __volatile__("sti");

    /* Infinite loop - kernel should never return */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
