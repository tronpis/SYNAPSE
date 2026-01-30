/* SYNAPSE SO - Fork System Call Implementation */
/* Licensed under GPLv3 */

#include <kernel/fork.h>
#include <kernel/process.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/heap.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <kernel/scheduler.h>

/* Fork system call implementation */
pid_t do_fork(void) {
    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[+] fork() called by process ");
    vga_print(current->name);
    vga_print(" (PID: ");
    vga_print_dec(current->pid);
    vga_print(")\n");

    /* Allocate new PCB for child */
    process_t* child = (process_t*)kmalloc(sizeof(process_t));
    if (child == 0) {
        vga_print("[-] fork: Failed to allocate child PCB\n");
        return -1;
    }

    /* Copy basic process information */
    child->pid = next_pid++;
    child->ppid = current->pid;
    strncpy(child->name, current->name, 31);
    child->name[31] = '\0';
    child->state = PROC_STATE_READY;
    child->flags = current->flags;
    child->exit_code = 0;
    child->priority = current->priority;
    child->quantum = current->quantum;

    /* Clone page directory with COW */
    child->page_dir = vmm_clone_page_directory(current->page_dir);
    if (child->page_dir == 0) {
        vga_print("[-] fork: Failed to clone page directory\n");
        kfree(child);
        return -1;
    }

    /* For user processes, allocate new stack */
    if (!(child->flags & PROC_FLAG_KERNEL)) {
        uint32_t stack_phys = pmm_alloc_frame();
        if (stack_phys == 0) {
            vga_print("[-] fork: Failed to allocate child stack\n");
            kfree(child);
            return -1;
        }

        uint32_t stack_virt = 0x7FFFF000;
        vmm_switch_page_directory(child->page_dir);
        vmm_map_page(stack_virt, stack_phys,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        vmm_switch_page_directory(current->page_dir);

        child->stack_start = stack_virt - USER_STACK_SIZE;
        child->stack_end = stack_virt;
    } else {
        /* Kernel threads share stack initially */
        child->stack_start = current->stack_start;
        child->stack_end = current->stack_end;
    }

    /* Copy CPU context */
    child->eip = current->eip;
    child->eflags = current->eflags;
    child->esp = current->esp;
    child->ebp = current->ebp;

    /* Copy registers */
    child->eax = 0;  /* Child returns 0 from fork */
    child->ebx = current->ebx;
    child->ecx = current->ecx;
    child->edx = current->edx;
    child->esi = current->esi;
    child->edi = current->edi;

    /* Copy heap boundaries */
    child->heap_start = current->heap_start;
    child->heap_end = current->heap_end;

    /* Add to process list */
    process_add_to_list(child);
    scheduler_add_process(child);

    vga_print("[+] fork: Created child process PID: ");
    vga_print_dec(child->pid);
    vga_print("\n");

    /* Parent returns child PID */
    return child->pid;
}

/* Vfork system call (simplified version - same as fork for now) */
pid_t sys_vfork(void) {
    return do_fork();
}
