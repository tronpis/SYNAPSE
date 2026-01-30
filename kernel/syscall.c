/* SYNAPSE SO - System Call Implementation */
/* Licensed under GPLv3 */

#include <kernel/syscall.h>
#include <kernel/vga.h>
#include <kernel/process.h>
#include <kernel/idt.h>
#include <kernel/string.h>
#include <kernel/vmm.h>
#include <kernel/fork.h>
#include <kernel/exec.h>
#include <kernel/wait.h>
#include <kernel/vfs.h>
#include <kernel/keyboard.h>

/* System call table */
static syscall_func_t syscall_table[NUM_SYSCALLS];

static int sys_exit_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_exit(arg1);
}

static int sys_write_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg4;
    (void)arg5;
    return sys_write(arg1, arg2, arg3);
}

static int sys_read_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg4;
    (void)arg5;
    return sys_read(arg1, arg2, arg3);
}

static int sys_open_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg4;
    (void)arg5;
    return sys_open(arg1, arg2, arg3);
}

static int sys_close_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_close(arg1);
}

static int sys_fork_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_fork();
}

static int sys_exec_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_exec(arg1, arg2);
}

static int sys_wait_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_wait(arg1, arg2);
}

static int sys_getpid_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return sys_getpid();
}

static int sys_lseek_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg4;
    (void)arg5;
    return sys_lseek((int)arg1, (int)arg2, (int)arg3);
}

/* Initialize system call interface */
void syscall_init(void) {
    vga_print("[+] Initializing System Call Interface...\n");

    /* Clear syscall table */
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        syscall_table[i] = 0;
    }

    /* Register system calls */
    syscall_register(SYS_EXIT, sys_exit_wrapper);
    syscall_register(SYS_WRITE, sys_write_wrapper);
    syscall_register(SYS_READ, sys_read_wrapper);
    syscall_register(SYS_OPEN, sys_open_wrapper);
    syscall_register(SYS_CLOSE, sys_close_wrapper);
    syscall_register(SYS_FORK, sys_fork_wrapper);
    syscall_register(SYS_EXEC, sys_exec_wrapper);
    syscall_register(SYS_WAIT, sys_wait_wrapper);
    syscall_register(SYS_GETPID, sys_getpid_wrapper);
    syscall_register(SYS_LSEEK, sys_lseek_wrapper);

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

uint32_t syscall_get_num(registers_t* regs) {
    if (regs == 0) {
        return 0;
    }

    return regs->eax;
}

void syscall_set_return(registers_t* regs, uint32_t value) {
    if (regs == 0) {
        return;
    }

    regs->eax = value;
}

/* System call handler (called from assembly) */
registers_t* syscall_handler(registers_t* regs) {
    if (regs == 0) {
        return 0;
    }

    /* Get syscall number */
    uint32_t num = syscall_get_num(regs);

    /* Check if syscall number is valid */
    if (num >= NUM_SYSCALLS || syscall_table[num] == 0) {
        vga_print("[-] Invalid syscall: ");
        vga_print_dec(num);
        vga_print("\n");
        syscall_set_return(regs, (uint32_t)-1);
        return regs;
    }

    /* Call syscall handler */
    syscall_func_t handler = syscall_table[num];
    uint32_t ret = handler(regs->ebx, regs->ecx, regs->edx, regs->esi,
                           regs->edi);

    /* Set return value */
    syscall_set_return(regs, ret);
    return regs;
}

/* System call implementations */

/* Exit the current process */
int sys_exit(uint32_t exit_code) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[process ");
    vga_print(current->name);
    vga_print(" exited]\n");

    process_exit((int)exit_code);
    return 0;
}

/* Write to a file descriptor */
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count) {
    (void)fd; /* File descriptor not used yet */

    /* Validate count */
    if (count == 0U) {
        return 0;
    }

    /* Reject kernel-space pointers to avoid leaking kernel memory */
    if (buffer >= 0xC0000000U) {
        return -1;
    }

    if (count > 4096U) {
        count = 4096U;
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
    /* Only basic stdin (fd=0) is supported for now. */
    if (fd != 0U) {
        return -1;
    }

    if (count == 0U) {
        return 0;
    }

    if (count > 4096U) {
        count = 4096U;
    }

    /* Reject kernel-space pointers to avoid corrupting kernel memory */
    if (buffer >= 0xC0000000U) {
        return -1;
    }

    uint32_t bytes_read = 0U;
    uint32_t user_addr = buffer;

    while (bytes_read < count) {
        if (keyboard_has_char() == 0) {
            break;
        }

        uint32_t user_page = user_addr & 0xFFFFF000U;
        uint32_t page_offset = user_addr & 0xFFFU;

        uint32_t phys_addr = vmm_get_phys_addr(user_page);
        if (phys_addr == 0U) {
            return (bytes_read > 0U) ? (int)bytes_read : -1;
        }

        int slot = vmm_alloc_temp_slot();
        if (slot < 0) {
            return (bytes_read > 0U) ? (int)bytes_read : -1;
        }

        uint32_t temp_virt = vmm_map_temp_page(phys_addr, slot);
        if (temp_virt == 0U) {
            vmm_free_temp_slot(slot);
            return (bytes_read > 0U) ? (int)bytes_read : -1;
        }

        uint32_t bytes_in_page = PAGE_SIZE - page_offset;
        uint32_t bytes_to_copy = count - bytes_read;
        if (bytes_to_copy > bytes_in_page) {
            bytes_to_copy = bytes_in_page;
        }

        char* mapped_buf = (char*)(temp_virt + page_offset);
        for (uint32_t i = 0U; (i < bytes_to_copy) && (bytes_read < count);
             i++) {
            if (keyboard_has_char() == 0) {
                break;
            }

            char c = keyboard_get_char();
            if (c == 0) {
                break;
            }

            mapped_buf[i] = c;
            bytes_read++;
            user_addr++;
        }

        vmm_unmap_temp_page(slot);
        vmm_free_temp_slot(slot);
    }

    return (int)bytes_read;
}

/* Open a file */
int sys_open(uint32_t filename, uint32_t flags, uint32_t mode) {
    /* Validate pointer is in user space */
    if (filename >= 0xC0000000U) {
        return -1;
    }

    const char* path = (const char*)filename;
    if (path == 0) {
        return -1;
    }

    return vfs_open(path, flags, mode);
}

/* Close a file descriptor */
int sys_close(uint32_t fd) {
    return vfs_close(fd);
}

/* Create a new process (fork) */
int sys_fork(void) {
    return (int)do_fork();
}

/* Execute a new program */
int sys_exec(uint32_t path, uint32_t argv) {
    /* Validate pointer is in user space */
    if (path >= 0xC0000000U) {
        return -1;
    }

    const char* prog_path = (const char*)path;
    char* const* argv_ptr = (char* const*)argv;

    return do_exec(prog_path, argv_ptr);
}

/* Wait for a process to exit */
int sys_wait(uint32_t pid, uint32_t status) {
    /* Validate status pointer is in user space */
    if (status >= 0xC0000000U) {
        return -1;
    }

    int* status_ptr = (int*)status;
    pid_t wait_pid = (pid_t)pid;

    return (int)do_wait(wait_pid, status_ptr);
}

/* Get current process ID */
int sys_getpid(void) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    return (int)current->pid;
}

/* Lseek system call */
int sys_lseek(int fd, int offset, int whence) {
    return vfs_lseek(fd, offset, whence);
}
