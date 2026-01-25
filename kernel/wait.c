/* SYNAPSE SO - Wait System Call Implementation */
/* Licensed under GPLv3 */

#include <kernel/wait.h>
#include <kernel/process.h>
#include <kernel/vga.h>
#include <kernel/scheduler.h>

/* Simple waiting list for processes waiting for children */
static process_t* wait_list = 0;

/* Wait system call implementation */
pid_t do_wait(pid_t pid, int* status) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[+] wait() called by process ");
    vga_print(current->name);
    vga_print(" (PID: ");
    vga_print_dec(current->pid);
    vga_print(")\n");

    /* Find child process */
    process_t* child = 0;
    process_t* proc = process_get_list();

    if (proc == 0) {
        vga_print("[-] wait: No processes in system\n");
        return -1;
    }

    /* Find zombie child */
    do {
        if (proc->ppid == current->pid && proc->state == PROC_STATE_ZOMBIE) {
            if (pid == -1 || proc->pid == pid) {
                child = proc;
                break;
            }
        }
        proc = proc->next;
    } while (proc != 0 && proc != process_get_list());

    if (child != 0) {
        /* Child has exited, return its status */
        vga_print("[+] wait: Child process ");
        vga_print(child->name);
        vga_print(" (PID: ");
        vga_print_dec(child->pid);
        vga_print(") found with status: ");
        vga_print_dec(child->exit_code);
        vga_print("\n");

        if (status != 0) {
            /* Validate status pointer is in user space */
            if ((uint32_t)status < 0xC0000000) {
                /* For now, we can't write to user space without temp mappings */
                /* In a real system, we'd use temporary mappings here */
            }
        }

        pid_t child_pid = child->pid;

        /* Destroy zombie child */
        process_destroy(child);

        return child_pid;
    }

    /* No zombie child found - block current process */
    /* In a real system, we'd use sleep/wakeup mechanism */
    vga_print("[-] wait: No child process found, blocking\n");

    /* For now, return error */
    return -1;
}
