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
#include <kernel/timer.h>
#include <kernel/pmm.h>
#include <kernel/scheduler.h>
#include <kernel/io.h>

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

static int sys_getppid_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                               uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_getppid();
}

static int sys_yield_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_yield();
}

static int sys_sleep_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_sleep(arg1);
}

static int sys_brk_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                           uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_brk(arg1);
}

static int sys_sbrk_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_sbrk((int32_t)arg1);
}

static int sys_stat_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_stat(arg1, arg2);
}

static int sys_fstat_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_fstat(arg1, arg2);
}

static int sys_kill_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_kill((int32_t)arg1, (int32_t)arg2);
}

static int sys_signal_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_signal((int32_t)arg1, arg2);
}

static int sys_uptime_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_uptime();
}

static int sys_sysinfo_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                               uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_sysinfo(arg1);
}

static int sys_getuid_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_getuid();
}

static int sys_setuid_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_setuid(arg1);
}

static int sys_dup_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                           uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_dup(arg1);
}

static int sys_pipe_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                            uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_pipe(arg1);
}

static int sys_chdir_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_chdir(arg1);
}

static int sys_getcwd_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_getcwd(arg1, arg2);
}

static int sys_mkdir_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    return sys_mkdir(arg1, arg2);
}

static int sys_rmdir_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                             uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_rmdir(arg1);
}

static int sys_unlink_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_unlink(arg1);
}

static int sys_gettime_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                               uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_gettime(arg1);
}

static int sys_reboot_wrapper(uint32_t arg1, uint32_t arg2, uint32_t arg3,
                              uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return sys_reboot(arg1);
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
    syscall_register(SYS_GETPPID, sys_getppid_wrapper);
    syscall_register(SYS_YIELD, sys_yield_wrapper);
    syscall_register(SYS_SLEEP, sys_sleep_wrapper);
    syscall_register(SYS_BRK, sys_brk_wrapper);
    syscall_register(SYS_SBRK, sys_sbrk_wrapper);
    syscall_register(SYS_STAT, sys_stat_wrapper);
    syscall_register(SYS_FSTAT, sys_fstat_wrapper);
    syscall_register(SYS_KILL, sys_kill_wrapper);
    syscall_register(SYS_SIGNAL, sys_signal_wrapper);
    syscall_register(SYS_UPTIME, sys_uptime_wrapper);
    syscall_register(SYS_SYSINFO, sys_sysinfo_wrapper);
    syscall_register(SYS_GETUID, sys_getuid_wrapper);
    syscall_register(SYS_SETUID, sys_setuid_wrapper);
    syscall_register(SYS_DUP, sys_dup_wrapper);
    syscall_register(SYS_PIPE, sys_pipe_wrapper);
    syscall_register(SYS_CHDIR, sys_chdir_wrapper);
    syscall_register(SYS_GETCWD, sys_getcwd_wrapper);
    syscall_register(SYS_MKDIR, sys_mkdir_wrapper);
    syscall_register(SYS_RMDIR, sys_rmdir_wrapper);
    syscall_register(SYS_UNLINK, sys_unlink_wrapper);
    syscall_register(SYS_GETTIME, sys_gettime_wrapper);
    syscall_register(SYS_REBOOT, sys_reboot_wrapper);

    vga_print("    System calls registered (32 syscalls)\n");
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

/* ============================================================================
 * New System Call Implementations
 * ============================================================================ */

/* Get parent process ID */
int sys_getppid(void) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }
    return (int)current->ppid;
}

/* Yield CPU to other processes */
int sys_yield(void) {
    schedule();
    return 0;
}

/* Sleep for specified ticks */
int sys_sleep(uint32_t ticks) {
    if (ticks == 0) {
        return 0;
    }

    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    uint32_t wake_tick = timer_get_ticks() + ticks;
    process_sleep_until(current, wake_tick);
    schedule();

    return 0;
}

/* Set program break */
int sys_brk(uint32_t addr) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }
    return (int)process_brk(current, addr);
}

/* Increment/decrement program break */
int sys_sbrk(int32_t increment) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }
    return process_sbrk(current, increment);
}

/* Get file statistics by path */
int sys_stat(uint32_t path, uint32_t statbuf) {
    /* Validate pointers */
    if (path >= 0xC0000000U || statbuf >= 0xC0000000U) {
        return -1;
    }

    /* For now, just return a basic stat structure */
    stat_t* st = (stat_t*)statbuf;
    if (st == 0) {
        return -1;
    }

    /* Initialize with defaults */
    st->st_dev = 0;
    st->st_ino = 1;
    st->st_mode = 0644;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = 0;
    st->st_blksize = 512;
    st->st_blocks = 0;
    st->st_atime = timer_get_uptime_seconds();
    st->st_mtime = timer_get_uptime_seconds();
    st->st_ctime = timer_get_uptime_seconds();

    return 0;
}

/* Get file statistics by fd */
int sys_fstat(uint32_t fd, uint32_t statbuf) {
    (void)fd;  /* Not fully implemented */

    /* Validate pointer */
    if (statbuf >= 0xC0000000U) {
        return -1;
    }

    stat_t* st = (stat_t*)statbuf;
    if (st == 0) {
        return -1;
    }

    /* Initialize with defaults */
    st->st_dev = 0;
    st->st_ino = 1;
    st->st_mode = 0644;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = 0;
    st->st_blksize = 512;
    st->st_blocks = 0;
    st->st_atime = timer_get_uptime_seconds();
    st->st_mtime = timer_get_uptime_seconds();
    st->st_ctime = timer_get_uptime_seconds();

    return 0;
}

/* Send signal to process */
int sys_kill(int32_t pid, int32_t sig) {
    if (sig < 0 || sig >= NSIG) {
        return -1;
    }

    /* Special case: pid == 0 means current process group (not implemented) */
    /* Special case: pid == -1 means all processes (not implemented) */
    if (pid <= 0) {
        return -1;
    }

    process_t* target = process_find_by_pid((pid_t)pid);
    if (target == 0) {
        return -1;
    }

    return process_send_signal(target, sig);
}

/* Set signal handler */
int sys_signal(int32_t signum, uint32_t handler) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    signal_handler_t old = process_set_signal_handler(current, signum,
                                                      (signal_handler_t)handler);
    return (int)(uint32_t)old;
}

/* Get system uptime in seconds */
int sys_uptime(void) {
    return (int)timer_get_uptime_seconds();
}

/* Get system information */
int sys_sysinfo(uint32_t info) {
    if (info >= 0xC0000000U) {
        return -1;
    }

    sysinfo_t* si = (sysinfo_t*)info;
    if (si == 0) {
        return -1;
    }

    si->uptime = timer_get_uptime_seconds();
    si->total_mem = pmm_get_total_frames() * PAGE_SIZE;
    si->free_mem = pmm_get_free_frames() * PAGE_SIZE;
    si->total_processes = process_count_total();
    si->running_processes = process_count_running();

    /* Get scheduler stats */
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    si->context_switches = stats.total_switches;

    return 0;
}

/* Get user ID */
int sys_getuid(void) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }
    return (int)process_get_uid(current);
}

/* Set user ID */
int sys_setuid(uint32_t uid) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    /* Only root (uid 0) can change UID */
    if (current->uid != 0 && current->euid != 0) {
        return -1;
    }

    process_set_uid(current, uid);
    return 0;
}

/* Duplicate file descriptor */
int sys_dup(uint32_t oldfd) {
    (void)oldfd;
    /* Not fully implemented - would need per-process fd table */
    return -1;
}

/* Create pipe */
int sys_pipe(uint32_t pipefd) {
    (void)pipefd;
    /* Not implemented - would need IPC infrastructure */
    return -1;
}

/* Change working directory */
int sys_chdir(uint32_t path) {
    if (path >= 0xC0000000U) {
        return -1;
    }

    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    const char* dir = (const char*)path;
    return process_set_cwd(current, dir);
}

/* Get current working directory */
int sys_getcwd(uint32_t buf, uint32_t size) {
    if (buf >= 0xC0000000U || size == 0) {
        return -1;
    }

    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    const char* cwd = process_get_cwd(current);
    uint32_t len = strlen(cwd);

    if (len + 1 > size) {
        return -1;
    }

    char* dest = (char*)buf;
    strncpy(dest, cwd, size - 1);
    dest[size - 1] = '\0';

    return (int)len;
}

/* Create directory */
int sys_mkdir(uint32_t path, uint32_t mode) {
    (void)path;
    (void)mode;
    /* Not implemented - would need VFS directory support */
    return -1;
}

/* Remove directory */
int sys_rmdir(uint32_t path) {
    (void)path;
    /* Not implemented - would need VFS directory support */
    return -1;
}

/* Unlink (delete) file */
int sys_unlink(uint32_t path) {
    (void)path;
    /* Not implemented - would need VFS file deletion support */
    return -1;
}

/* Get system time */
int sys_gettime(uint32_t timeval) {
    if (timeval >= 0xC0000000U) {
        return -1;
    }

    /* Simple time structure: seconds, milliseconds */
    uint32_t* tv = (uint32_t*)timeval;
    if (tv == 0) {
        return -1;
    }

    tv[0] = timer_get_uptime_seconds();
    tv[1] = timer_get_uptime_ms() % 1000;

    return 0;
}

/* Reboot/shutdown system */
int sys_reboot(uint32_t cmd) {
    process_t* current = process_get_current();

    /* Only root can reboot */
    if (current != 0 && current->uid != 0 && current->euid != 0) {
        return -1;
    }

    vga_print("\n[REBOOT] System reboot requested\n");

    switch (cmd) {
        case REBOOT_CMD_RESTART:
            vga_print("[REBOOT] Restarting system...\n");
            /* Triple fault to reboot - write to 8042 keyboard controller */
            outb(0x64, 0xFE);
            break;

        case REBOOT_CMD_HALT:
            vga_print("[REBOOT] System halted.\n");
            __asm__ __volatile__("cli; hlt");
            break;

        case REBOOT_CMD_POWEROFF:
            vga_print("[REBOOT] Power off requested.\n");
            /* ACPI power off not implemented - just halt */
            __asm__ __volatile__("cli; hlt");
            break;

        default:
            return -1;
    }

    /* Should not reach here */
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return 0;
}
