/* SYNAPSE SO - System Call Implementation */
/* Licensed under GPLv3 */

#include <kernel/syscall.h>
#include <kernel/vga.h>
#include <kernel/process.h>
#include <kernel/idt.h>
#include <kernel/string.h>
#include <kernel/vmm.h>

/* System call table */
static syscall_func_t syscall_table[NUM_SYSCALLS];

/* Initialize system call interface */
void syscall_init(void) {
    vga_print("[+] Initializing System Call Interface...\n");

    /* Clear syscall table */
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        syscall_table[i] = 0;
    }

    /* Register system calls */
    syscall_register(SYS_EXIT, (syscall_func_t)sys_exit);
    syscall_register(SYS_WRITE, (syscall_func_t)sys_write);
    syscall_register(SYS_READ, (syscall_func_t)sys_read);
    syscall_register(SYS_OPEN, (syscall_func_t)sys_open);
    syscall_register(SYS_CLOSE, (syscall_func_t)sys_close);
    syscall_register(SYS_FORK, (syscall_func_t)sys_fork);
    syscall_register(SYS_EXEC, (syscall_func_t)sys_exec);
    syscall_register(SYS_WAIT, (syscall_func_t)sys_wait);
    syscall_register(SYS_GETPID, (syscall_func_t)sys_getpid);

    vga_print("    System calls registered\n");
}

/* Register a system call handler */
void syscall_register(uint32_t num, syscall_func_t handler) {
    if (num >= NUM_SYSCALLS) {
        vga_print("[-] Invalid syscall number: ");
        vga_print_dec(num);
        vga_print("\n");
        return;
    }

    syscall_table[num] = handler;
}

/* System call handler (called from assembly) */
void syscall_handler(registers_t* regs) {
    /* Get syscall number */
    uint32_t num = syscall_get_num(regs);

    /* Check if syscall number is valid */
    if (num >= NUM_SYSCALLS || syscall_table[num] == 0) {
        vga_print("[-] Invalid syscall: ");
        vga_print_dec(num);
        vga_print("\n");
        syscall_set_return(regs, -1);
        return;
    }

    /* Call syscall handler */
    syscall_func_t handler = syscall_table[num];
    uint32_t ret = handler(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

    /* Set return value */
    syscall_set_return(regs, ret);
}

/* System call implementations */

/* Exit the current process */
int sys_exit(uint32_t exit_code) {
    (void)exit_code; /* Parameter not used yet */

    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[process ");
    vga_print(current->name);
    vga_print(" exited]\n");

    process_exit(current);
    return 0;
}

/* Write to a file descriptor */
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count) {
    (void)fd; /* File descriptor not used yet */

    /* Validate count */
    if (count == 0) {
        return 0;
    }
    
    /* Maximum write size to prevent excessive iterations */
    if (count > 4096) {
        count = 4096;
    }

    /* Access user buffer safely using temporary mappings */
    uint32_t bytes_written = 0;
    uint32_t user_addr = buffer;
    
    while (bytes_written < count) {
        /* Get physical address of user page */
        uint32_t user_page = user_addr & 0xFFFFF000;
        uint32_t page_offset = user_addr & 0xFFF;
        
        /* Get physical address (this validates the page is mapped) */
        uint32_t phys_addr = vmm_get_phys_addr(user_page);
        if (phys_addr == 0) {
            /* Page not mapped - invalid pointer */
            return bytes_written > 0 ? (int)bytes_written : -1;
        }
        
        /* Allocate temporary mapping slot */
        int slot = vmm_alloc_temp_slot();
        if (slot < 0) {
            /* No temporary slots available */
            return bytes_written > 0 ? (int)bytes_written : -1;
        }
        
        /* Map user page temporarily to kernel space */
        uint32_t temp_virt = vmm_map_temp_page(phys_addr, slot);
        if (temp_virt == 0) {
            vmm_free_temp_slot(slot);
            return bytes_written > 0 ? (int)bytes_written : -1;
        }
        
        /* Calculate how many bytes we can read from this page */
        uint32_t bytes_in_page = PAGE_SIZE - page_offset;
        uint32_t bytes_to_write = count - bytes_written;
        if (bytes_to_write > bytes_in_page) {
            bytes_to_write = bytes_in_page;
        }
        
        /* Write characters from mapped page */
        char* mapped_buf = (char*)(temp_virt + page_offset);
        for (uint32_t i = 0; i < bytes_to_write; i++) {
            vga_put_char(mapped_buf[i]);
        }
        
        /* Cleanup temporary mapping */
        vmm_unmap_temp_page(slot);
        vmm_free_temp_slot(slot);
        
        /* Advance to next page */
        bytes_written += bytes_to_write;
        user_addr += bytes_to_write;
    }

    return (int)bytes_written;
}

/* Read from a file descriptor */
int sys_read(uint32_t fd, uint32_t buffer, uint32_t count) {
    (void)fd; /* File descriptor not used yet */
    (void)buffer;
    (void)count;

    /* Not implemented yet */
    return -1;
}

/* Open a file */
int sys_open(uint32_t filename, uint32_t flags, uint32_t mode) {
    (void)filename; /* Not implemented yet */
    (void)flags;
    (void)mode;

    /* Not implemented yet - file system needed */
    return -1;
}

/* Close a file descriptor */
int sys_close(uint32_t fd) {
    (void)fd; /* Not implemented yet */

    /* Not implemented yet - file system needed */
    return -1;
}

/* Create a new process (fork) */
int sys_fork(void) {
    /* Not fully implemented yet */
    /* This is a stub for Phase 3 */
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[+] sys_fork called (stub)\n");
    return -1; /* Return -1 to indicate not implemented */
}

/* Execute a new program */
int sys_exec(uint32_t path, uint32_t argv) {
    (void)path; /* Not implemented yet */
    (void)argv;

    /* Not implemented yet - needs ELF loader integration */
    vga_print("[+] sys_exec called (stub)\n");
    return -1;
}

/* Wait for a process to exit */
int sys_wait(uint32_t pid, uint32_t status) {
    (void)pid; /* Not implemented yet */
    (void)status;

    /* Not implemented yet */
    vga_print("[+] sys_wait called (stub)\n");
    return -1;
}

/* Get current process ID */
int sys_getpid(void) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    return (int)current->pid;
}
