/* SYNAPSE SO - Process Management Implementation */
/* Licensed under GPLv3 */

#include <kernel/process.h>
#include <kernel/gdt.h>
#include <kernel/heap.h>
#include <kernel/pmm.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <kernel/vmm.h>
#include <kernel/const.h>
#include <kernel/scheduler.h>

#define IRQ0_VECTOR       32

/* Process list */
process_t* process_list = 0;
static process_t* current_process = 0;

/* Next PID to assign (extern for fork.c) */
pid_t next_pid = 1;

static void process_list_insert(process_t* proc) {
    unsigned int flags;
    /* Save EFLAGS and disable interrupts to make the insertion atomic.
       This avoids list corruption if an interrupt handler or another CPU
       traverses/modifies `process_list`. */
    asm volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");

    if (process_list == 0) {
        process_list = proc;
        proc->next = proc;
        proc->prev = proc;
    } else {
        proc->next = process_list;
        proc->prev = process_list->prev;
        process_list->prev->next = proc;
        process_list->prev = proc;
    }

    /* Restore interrupts to previous state (IF flag). */
    if (flags & (1 << 9)) {
        asm volatile("sti");
    }
}

void process_add_to_list(process_t* proc) {
    if (proc == 0) {
        return;
    }

    proc->next = 0;
    proc->prev = 0;
    process_list_insert(proc);
}

static uint32_t* stack_push(uint32_t* sp, uint32_t value) {
    sp--;
    *sp = value;
    return sp;
}

static void process_init_kernel_thread_stack(process_t* proc,
                                            process_entry_t entry) {
    uint32_t* sp = (uint32_t*)proc->stack_end;

    /* iret frame */
    sp = stack_push(sp, 0x202); /* EFLAGS (IF=1) */
    sp = stack_push(sp, GDT_KERNEL_CODE);
    sp = stack_push(sp, (uint32_t)entry);

    /* int_no + err_code (like IRQ stubs) */
    sp = stack_push(sp, 0);
    sp = stack_push(sp, IRQ0_VECTOR);

    /* pusha frame (matches registers_t order after pusha) */
    sp = stack_push(sp, 0); /* eax */
    sp = stack_push(sp, 0); /* ecx */
    sp = stack_push(sp, 0); /* edx */
    sp = stack_push(sp, 0); /* ebx */
    sp = stack_push(sp, 0); /* esp (ignored by popa) */
    sp = stack_push(sp, 0); /* ebp */
    sp = stack_push(sp, 0); /* esi */
    sp = stack_push(sp, 0); /* edi */

    /* segment registers (push order in isr_common_stub: ds, es, fs, gs) */
    sp = stack_push(sp, GDT_KERNEL_DATA); /* ds */
    sp = stack_push(sp, GDT_KERNEL_DATA); /* es */
    sp = stack_push(sp, GDT_KERNEL_DATA); /* fs */
    sp = stack_push(sp, GDT_KERNEL_DATA); /* gs */

    proc->esp = (uint32_t)sp;
}

/* Initialize process management */
void process_init(void) {
    vga_print("[+] Initializing Process Management...\n");
    process_list = 0;
    current_process = 0;
    next_pid = 1;
}

process_t* process_create_current(const char* name) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (proc == 0) {
        return 0;
    }

    proc->pid = next_pid++;
    proc->ppid = 0;
    proc->state = PROC_STATE_RUNNING;
    proc->flags = PROC_FLAG_KERNEL;

    proc->page_dir = vmm_get_current_directory();
    proc->heap_start = 0;
    proc->heap_end = 0;
    proc->stack_start = 0;
    proc->stack_end = 0;

    proc->esp = 0;
    proc->ebp = 0;
    proc->eip = 0;
    proc->eflags = 0x202;

    proc->eax = proc->ebx = proc->ecx = proc->edx = 0;
    proc->esi = proc->edi = 0;

    proc->exit_code = 0;
    proc->priority = PRIORITY_HIGH;  /* Kernel processes get high priority */
    proc->quantum = 0;
    proc->brk = 0;

    /* Initialize UID/GID to root for kernel */
    proc->uid = proc->gid = 0;
    proc->euid = proc->egid = 0;

    /* Initialize signal handling */
    proc->pending_signals = 0;
    for (int i = 0; i < NSIG; i++) {
        proc->signal_handlers[i] = SIG_DFL;
    }

    /* Initialize working directory */
    proc->cwd[0] = '/';
    proc->cwd[1] = '\0';

    /* Initialize sleep management */
    proc->wake_tick = 0;

    if (name != 0) {
        strncpy(proc->name, name, 31);
        proc->name[31] = '\0';
    } else {
        strcpy(proc->name, "kernel");
    }

    process_list_insert(proc);
    current_process = proc;

    vga_print("    Created current process: ");
    vga_print(proc->name);
    vga_print(" (PID: ");
    vga_print_dec(proc->pid);
    vga_print(")\n");

    return proc;
}

/* Create a new process */
process_t* process_create(const char* name, uint32_t flags,
                          process_entry_t entry) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (proc == 0) {
        return 0;
    }

    proc->pid = next_pid++;
    proc->ppid = (current_process != 0) ? current_process->pid : 0;
    proc->state = PROC_STATE_READY;
    proc->flags = flags;

    proc->exit_code = 0;
    proc->priority = PRIORITY_NORMAL;  /* Default priority */
    proc->quantum = 10;

    proc->heap_start = 0;
    proc->heap_end = 0;
    proc->brk = 0;

    /* Inherit UID/GID from parent or default to 0 */
    if (current_process != 0) {
        proc->uid = current_process->uid;
        proc->gid = current_process->gid;
        proc->euid = current_process->euid;
        proc->egid = current_process->egid;
    } else {
        proc->uid = proc->gid = 0;
        proc->euid = proc->egid = 0;
    }

    /* Initialize signal handling */
    proc->pending_signals = 0;
    for (int i = 0; i < NSIG; i++) {
        proc->signal_handlers[i] = SIG_DFL;
    }

    /* Inherit working directory from parent or default to "/" */
    if (current_process != 0 && current_process->cwd[0] != '\0') {
        strncpy(proc->cwd, current_process->cwd, 255);
        proc->cwd[255] = '\0';
    } else {
        proc->cwd[0] = '/';
        proc->cwd[1] = '\0';
    }

    /* Initialize sleep management */
    proc->wake_tick = 0;

    if (name != 0) {
        strncpy(proc->name, name, 31);
        proc->name[31] = '\0';
    } else {
        strcpy(proc->name, "unknown");
    }

    /* Create page directory */
    if (flags & PROC_FLAG_KERNEL) {
        proc->page_dir = vmm_get_current_directory();
    } else {
        proc->page_dir = vmm_create_page_directory();
        if (proc->page_dir == 0) {
            kfree(proc);
            return 0;
        }
    }

    uint32_t stack_size = (flags & PROC_FLAG_KERNEL) ? KERNEL_STACK_SIZE :
                          USER_STACK_SIZE;

    if (flags & PROC_FLAG_KERNEL) {
        void* stack = kmalloc(stack_size);
        if (stack == 0) {
            kfree(proc);
            return 0;
        }

        proc->stack_start = (uint32_t)stack;
        proc->stack_end = proc->stack_start + stack_size;
    } else {
        uint32_t stack_phys = pmm_alloc_frame();
        if (stack_phys == 0) {
            kfree(proc);
            return 0;
        }

        uint32_t stack_virt = 0x7FFFF000;
        vmm_map_page(stack_virt, stack_phys,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        proc->stack_start = stack_virt - stack_size;
        proc->stack_end = stack_virt;
    }

    proc->eip = (uint32_t)entry;
    proc->eflags = 0x202;

    proc->eax = proc->ebx = proc->ecx = proc->edx = 0;
    proc->esi = proc->edi = 0;

    if ((flags & PROC_FLAG_KERNEL) && entry != 0) {
        process_init_kernel_thread_stack(proc, entry);
    } else {
        proc->esp = proc->stack_end;
        proc->ebp = proc->stack_end;
    }

    process_list_insert(proc);

    vga_print("    Created process: ");
    vga_print(proc->name);
    vga_print(" (PID: ");
    vga_print_dec(proc->pid);
    vga_print(")\n");

    return proc;
}

/* Destroy a process */
void process_destroy(process_t* proc) {
    if (proc == 0) {
        return;
    }

    /* Disable interrupts to make the list removal and cleanup atomic.
       Keep interrupts disabled until after kfree to prevent use-after-free
       if an interrupt handler accesses the process being destroyed. */
    unsigned int flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");

    if (proc->next == proc) {
        process_list = 0;
    } else {
        proc->next->prev = proc->prev;
        proc->prev->next = proc->next;
        if (process_list == proc) {
            process_list = proc->next;
        }
    }

    if (proc == current_process) {
        current_process = 0;
    }

    /* Free the process stack and structure while interrupts are disabled.
       This prevents ISR from accessing freed memory. */
    if ((proc->flags & PROC_FLAG_KERNEL) && proc->stack_start != 0) {
        kfree((void*)proc->stack_start);
    }

    kfree(proc);

    /* Restore interrupts after all cleanup is complete */
    if (flags & (1 << 9)) {
        asm volatile("sti");
    }
}

/* Get current process */
process_t* process_get_current(void) {
    return current_process;
}

void process_set_current(process_t* proc) {
    current_process = proc;
}

/* Get process list */
process_t* process_get_list(void) {
    return process_list;
}

/* Find process by PID */
process_t* process_find_by_pid(pid_t pid) {
    if (process_list == 0) {
        return 0;
    }

    process_t* proc = process_list;
    do {
        if (proc->pid == pid) {
            return proc;
        }
        proc = proc->next;
    } while (proc != 0 && proc != process_list);

    return 0;
}

/* Set process state */
void process_set_state(process_t* proc, uint32_t state) {
    if (proc != 0) {
        proc->state = state;
    }
}

/* Make process ready */
void process_ready(process_t* proc) {
    if (proc != 0) {
        proc->state = PROC_STATE_READY;
        scheduler_add_process(proc);
    }
}

/* Block a process */
void process_block(process_t* proc) {
    if (proc != 0) {
        proc->state = PROC_STATE_BLOCKED;
        scheduler_remove_process(proc);
    }
}

/* Unblock a process */
void process_unblock(process_t* proc) {
    if (proc != 0) {
        proc->state = PROC_STATE_READY;
        scheduler_add_process(proc);
    }
}

/* Execute an ELF binary */
int process_exec(uint8_t* elf_data, uint32_t size) {
    (void)elf_data;
    (void)size;
    vga_print("[-] ELF loader not yet implemented\n");
    return -1;
}

/* Exit current process */
void process_exit(int exit_code) {
    if (current_process == 0) {
        return;
    }

    current_process->state = PROC_STATE_ZOMBIE;
    current_process->exit_code = (uint32_t)exit_code;

    vga_print("Process exited: ");
    vga_print(current_process->name);
    vga_print(" (PID: ");
    vga_print_dec(current_process->pid);
    vga_print(", exit code: ");
    vga_print_dec((unsigned int)exit_code);
    vga_print(")\n");

    /* Yield; a reaper process can clean this up in a later phase. */
    schedule();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/* Get current PID */
pid_t process_get_pid(void) {
    return (current_process != 0) ? current_process->pid : 0;
}

/* Get parent PID */
pid_t process_get_ppid(void) {
    return (current_process != 0) ? current_process->ppid : 0;
}

/* Set process name */
void process_set_name(process_t* proc, const char* name) {
    if (proc != 0 && name != 0) {
        strncpy(proc->name, name, 31);
        proc->name[31] = '\0';
    }
}

/* Idle process */
void idle_process(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* ============================================================================
 * Signal Handling
 * ============================================================================ */

/* Send a signal to a process */
int process_send_signal(process_t* proc, int signum) {
    if (proc == 0 || signum < 1 || signum >= NSIG) {
        return -1;
    }

    /* Set the signal as pending */
    proc->pending_signals |= (1U << signum);

    /* If process is blocked/sleeping, wake it for signal delivery */
    if (proc->state == PROC_STATE_BLOCKED) {
        proc->state = PROC_STATE_READY;
    }

    return 0;
}

/* Set signal handler for a process */
signal_handler_t process_set_signal_handler(process_t* proc, int signum,
                                            signal_handler_t handler) {
    if (proc == 0 || signum < 1 || signum >= NSIG) {
        return SIG_ERR;
    }

    /* SIGKILL and SIGSTOP cannot be caught or ignored */
    if (signum == 9 || signum == 19) {
        return SIG_ERR;
    }

    signal_handler_t old_handler = proc->signal_handlers[signum];
    proc->signal_handlers[signum] = handler;
    return old_handler;
}

/* Check and handle pending signals */
void process_check_signals(process_t* proc) {
    if (proc == 0 || proc->pending_signals == 0) {
        return;
    }

    for (int sig = 1; sig < NSIG; sig++) {
        if (!(proc->pending_signals & (1U << sig))) {
            continue;
        }

        /* Clear the pending signal */
        proc->pending_signals &= ~(1U << sig);

        /* Handle SIGKILL - always terminates */
        if (sig == 9) {
            proc->state = PROC_STATE_ZOMBIE;
            proc->exit_code = 128 + sig;
            return;
        }

        /* Handle SIGSTOP - always stops */
        if (sig == 19) {
            proc->state = PROC_STATE_STOPPED;
            return;
        }

        /* Handle SIGCONT - resume if stopped */
        if (sig == 18) {
            if (proc->state == PROC_STATE_STOPPED) {
                proc->state = PROC_STATE_READY;
            }
            continue;
        }

        /* Get the signal handler */
        signal_handler_t handler = proc->signal_handlers[sig];

        if (handler == SIG_IGN) {
            /* Signal is ignored */
            continue;
        }

        if (handler == SIG_DFL || handler == 0) {
            /* Default action - terminate for most signals */
            switch (sig) {
                case 1:  /* SIGHUP */
                case 2:  /* SIGINT */
                case 3:  /* SIGQUIT */
                case 4:  /* SIGILL */
                case 6:  /* SIGABRT */
                case 8:  /* SIGFPE */
                case 11: /* SIGSEGV */
                case 13: /* SIGPIPE */
                case 15: /* SIGTERM */
                    proc->state = PROC_STATE_ZOMBIE;
                    proc->exit_code = 128 + (uint32_t)sig;
                    return;

                case 17: /* SIGCHLD - default is ignore */
                default:
                    /* Ignore */
                    break;
            }
        }

        /* TODO: User-defined signal handlers require user mode support */
    }
}

/* ============================================================================
 * User/Group ID Management
 * ============================================================================ */

uint32_t process_get_uid(process_t* proc) {
    return (proc != 0) ? proc->uid : 0;
}

void process_set_uid(process_t* proc, uint32_t uid) {
    if (proc != 0) {
        proc->uid = uid;
        proc->euid = uid;
    }
}

uint32_t process_get_gid(process_t* proc) {
    return (proc != 0) ? proc->gid : 0;
}

void process_set_gid(process_t* proc, uint32_t gid) {
    if (proc != 0) {
        proc->gid = gid;
        proc->egid = gid;
    }
}

/* ============================================================================
 * Program Break (brk/sbrk)
 * ============================================================================ */

uint32_t process_brk(process_t* proc, uint32_t addr) {
    if (proc == 0) {
        return 0;
    }

    /* Initialize brk if not set */
    if (proc->brk == 0) {
        proc->brk = proc->heap_end > 0 ? proc->heap_end : 0x10000000;
    }

    /* If addr is 0, just return current brk */
    if (addr == 0) {
        return proc->brk;
    }

    /* Validate new address */
    if (addr < proc->heap_start && proc->heap_start != 0) {
        return proc->brk;  /* Cannot shrink below heap start */
    }

    /* Don't allow growing into stack */
    if (addr >= proc->stack_start && proc->stack_start != 0) {
        return proc->brk;
    }

    /* Update brk */
    proc->brk = addr;
    return proc->brk;
}

int32_t process_sbrk(process_t* proc, int32_t increment) {
    if (proc == 0) {
        return -1;
    }

    /* Initialize brk if not set */
    if (proc->brk == 0) {
        proc->brk = proc->heap_end > 0 ? proc->heap_end : 0x10000000;
    }

    uint32_t old_brk = proc->brk;
    uint32_t new_brk;

    if (increment >= 0) {
        new_brk = old_brk + (uint32_t)increment;
        /* Check for overflow */
        if (new_brk < old_brk) {
            return -1;
        }
    } else {
        uint32_t abs_inc = (uint32_t)(-increment);
        if (abs_inc > old_brk) {
            return -1;  /* Underflow */
        }
        new_brk = old_brk - abs_inc;
    }

    /* Don't allow growing into stack */
    if (new_brk >= proc->stack_start && proc->stack_start != 0) {
        return -1;
    }

    proc->brk = new_brk;
    return (int32_t)old_brk;
}

/* ============================================================================
 * Working Directory
 * ============================================================================ */

const char* process_get_cwd(process_t* proc) {
    if (proc == 0) {
        return "/";
    }

    /* Return default if not set */
    if (proc->cwd[0] == '\0') {
        return "/";
    }

    return proc->cwd;
}

int process_set_cwd(process_t* proc, const char* path) {
    if (proc == 0 || path == 0) {
        return -1;
    }

    /* Validate path length */
    uint32_t len = strlen(path);
    if (len >= 255) {
        return -1;
    }

    strncpy(proc->cwd, path, 255);
    proc->cwd[255] = '\0';
    return 0;
}

/* ============================================================================
 * Sleep Management
 * ============================================================================ */

void process_sleep_until(process_t* proc, uint32_t wake_tick) {
    if (proc == 0) {
        return;
    }

    proc->wake_tick = wake_tick;
    proc->state = PROC_STATE_BLOCKED;
}

void process_check_sleeping(void) {
    /* This function is called from the scheduler tick to wake sleeping procs */
    if (process_list == 0) {
        return;
    }

    /* Import timer_get_ticks - forward declaration to avoid circular deps */
    extern uint32_t timer_get_ticks(void);
    uint32_t current_tick = timer_get_ticks();

    process_t* proc = process_list;
    process_t* start = proc;

    do {
        if (proc->state == PROC_STATE_BLOCKED && proc->wake_tick != 0) {
            if (current_tick >= proc->wake_tick) {
                proc->wake_tick = 0;
                proc->state = PROC_STATE_READY;
            }
        }
        proc = proc->next;
    } while (proc != 0 && proc != start);
}

/* ============================================================================
 * Process Counting
 * ============================================================================ */

uint32_t process_count_total(void) {
    if (process_list == 0) {
        return 0;
    }

    uint32_t count = 0;
    process_t* proc = process_list;
    process_t* start = proc;

    do {
        count++;
        proc = proc->next;
    } while (proc != 0 && proc != start);

    return count;
}

uint32_t process_count_running(void) {
    if (process_list == 0) {
        return 0;
    }

    uint32_t count = 0;
    process_t* proc = process_list;
    process_t* start = proc;

    do {
        if (proc->state == PROC_STATE_RUNNING || 
            proc->state == PROC_STATE_READY) {
            count++;
        }
        proc = proc->next;
    } while (proc != 0 && proc != start);

    return count;
}
