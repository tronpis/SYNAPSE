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
    vga_print("[+] Creating user mode test process...\n");
    
    /* Calculate size of user code */
    uint32_t code_size = (uint32_t)user_test_code_end - (uint32_t)user_test_code;
    if (code_size > PAGE_SIZE * 4) {
        vga_print("[-] User code too large\n");
        return 0;
    }
    
    vga_print("    Code size: ");
    vga_print_dec(code_size);
    vga_print(" bytes\n");
    
    /* Create process */
    process_t* proc = process_create("user_test", 0);
    if (proc == 0) {
        vga_print("[-] Failed to create process\n");
        return 0;
    }

    /* Mark as user mode process */
    proc->flags = PROC_FLAG_USER;
    
    /* Allocate user space memory at 0x400000 (4MB) */
    uint32_t user_code_virt = 0x400000;
    uint32_t user_stack_virt = 0x800000;  /* Stack at 8MB */
    
    /* Allocate and map code pages */
    uint32_t pages_needed = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t phys = pmm_alloc_frame();
        if (phys == 0) {
            vga_print("[-] Failed to allocate code page\n");
            process_destroy(proc);
            return 0;
        }
        
        /* Map to process address space with user permissions */
        page_directory_t* old_dir = vmm_get_current_directory();
        vmm_switch_page_directory(proc->page_dir);
        vmm_map_page(user_code_virt + (i * PAGE_SIZE), phys,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        vmm_switch_page_directory(old_dir);
    }
    
    /* Allocate and map stack pages (4 pages = 16KB stack) */
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t phys = pmm_alloc_frame();
        if (phys == 0) {
            vga_print("[-] Failed to allocate stack page\n");
            process_destroy(proc);
            return 0;
        }
        
        page_directory_t* old_dir = vmm_get_current_directory();
        vmm_switch_page_directory(proc->page_dir);
        vmm_map_page(user_stack_virt - (i * PAGE_SIZE), phys,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        vmm_switch_page_directory(old_dir);
    }
    
    /* Copy code to user space using temporary mapping */
    page_directory_t* old_dir = vmm_get_current_directory();
    vmm_switch_page_directory(proc->page_dir);
    
    for (uint32_t offset = 0; offset < code_size; offset += PAGE_SIZE) {
        uint32_t virt = user_code_virt + offset;
        uint32_t phys = vmm_get_phys_addr(virt & 0xFFFFF000);
        
        vmm_switch_page_directory(old_dir);
        
        int slot = vmm_alloc_temp_slot();
        if (slot < 0) {
            vga_print("[-] Failed to allocate temp slot\n");
            vmm_switch_page_directory(old_dir);
            process_destroy(proc);
            return 0;
        }
        
        uint32_t temp_virt = vmm_map_temp_page(phys, slot);
        if (temp_virt == 0) {
            vga_print("[-] Failed to map temp page\n");
            vmm_free_temp_slot(slot);
            vmm_switch_page_directory(old_dir);
            process_destroy(proc);
            return 0;
        }
        
        /* Copy code chunk */
        uint32_t bytes_to_copy = code_size - offset;
        if (bytes_to_copy > PAGE_SIZE) {
            bytes_to_copy = PAGE_SIZE;
        }
        
        uint8_t* src = (uint8_t*)((uint32_t)user_test_code + offset);
        uint8_t* dst = (uint8_t*)(temp_virt + (virt & 0xFFF));
        
        for (uint32_t i = 0; i < bytes_to_copy; i++) {
            dst[i] = src[i];
        }
        
        vmm_unmap_temp_page(slot);
        vmm_free_temp_slot(slot);
        
        vmm_switch_page_directory(proc->page_dir);
    }
    
    vmm_switch_page_directory(old_dir);
    
    /* Set up initial process state */
    proc->eip = user_code_virt;
    proc->esp = user_stack_virt;  /* Stack grows down */
    proc->user_stack = user_stack_virt;
    proc->state = PROC_STATE_READY;
    proc->priority = PRIORITY_NORMAL;  /* User processes get normal priority */
    
    vga_print("    Process created: PID ");
    vga_print_dec(proc->pid);
    vga_print("\n");
    vga_print("    Code at: 0x");
    vga_print_hex(user_code_virt);
    vga_print("\n");
    vga_print("    Stack at: 0x");
    vga_print_hex(user_stack_virt);
    vga_print("\n");
    
    /* Add to scheduler */
    scheduler_add_process(proc);
    
    return proc->pid;
}
