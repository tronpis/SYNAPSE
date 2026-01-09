/* SYNAPSE SO - Scheduler Implementation */
/* Licensed under GPLv3 */

/* SMP/Multicore Note:
 * This scheduler uses interrupt disable (cli) for process_list synchronization,
 * which is correct for single-core x86 systems. For SMP/multicore support,
 * spinlocks or proper atomic primitives would be needed instead of/cli.
 * Current assumption: Uniprocessor system. */

#include <kernel/scheduler.h>
#include <kernel/process.h>
#include <kernel/vga.h>
#include <kernel/vmm.h>

/* Scheduler quantum */
static uint32_t quantum = DEFAULT_QUANTUM;

static int proc_is_runnable(const process_t* proc) {
    if (proc == 0) {
        return 0;
    }

    if (proc->state == PROC_STATE_BLOCKED || proc->state == PROC_STATE_ZOMBIE ||
        proc->state == PROC_STATE_STOPPED) {
        return 0;
    }

    return 1;
}

static process_t* scheduler_pick_next(process_t* current) {
    if (process_list == 0) {
        return 0;
    }

    process_t* start = (current != 0 && current->next != 0) ? current->next :
                       process_list;
    process_t* proc = start;

    do {
        if (proc_is_runnable(proc)) {
            return proc;
        }

        proc = proc->next;
    } while (proc != 0 && proc != start);

    return current;
}

/* Initialize scheduler */
void scheduler_init(void) {
    vga_print("[+] Initializing Scheduler...\n");
    quantum = DEFAULT_QUANTUM;
    vga_print("    Scheduler ready\n");
}

/* Add process to scheduler */
void scheduler_add_process(process_t* proc) {
    if (proc == 0) {
        return;
    }

    if (proc->state != PROC_STATE_ZOMBIE && proc->state != PROC_STATE_STOPPED) {
        proc->state = PROC_STATE_READY;
    }

    proc->quantum = quantum;
}

/* Remove process from scheduler */
void scheduler_remove_process(process_t* proc) {
    if (proc == 0) {
        return;
    }

    proc->state = PROC_STATE_STOPPED;
}

/* Schedule next process (called by timer interrupt) */
registers_t* scheduler_tick(registers_t* regs) {
    /* Disable interrupts to protect process_list access.
       Called from IRQ context but process_list can be modified from
       other contexts (e.g., process_destroy, process_create). */
    unsigned int flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");

    process_t* current = process_get_current();

    /* If the recorded current process is not runnable (e.g. blocked/stopped),
       treat it as no current so the scheduler can pick a runnable process. */
    if (current != 0 && !proc_is_runnable(current)) {
        /* Clear current so the existing initialization path runs and selects
           a runnable process (if any). Also clear global current pointer. */
        process_set_current(0);
        current = 0;
    }

    if (current == 0) {
        if (process_list == 0) {
            /* Restore interrupts before returning */
            if (flags & (1 << 9)) {
                asm volatile("sti");
            }
            return regs;
        }

        process_set_current(process_list);
        process_list->state = PROC_STATE_RUNNING;
        process_list->quantum = quantum;
        current = process_list;
    }

    /* Restore interrupts for context switch operations */
    if (flags & (1 << 9)) {
        asm volatile("sti");
    }

    /* Save the current interrupt frame pointer as the process context */
    current->esp = (uint32_t)regs;

    if (current->quantum > 0) {
        current->quantum--;
    }

    if (current->quantum > 0) {
        return regs;
    }

    current->quantum = quantum;

    process_t* next = scheduler_pick_next(current);
    if (next == 0 || next == current) {
        return regs;
    }

    if (next->esp == 0) {
        return regs;
    }

    if (current->state == PROC_STATE_RUNNING) {
        current->state = PROC_STATE_READY;
    }

    next->state = PROC_STATE_RUNNING;
    if (next->quantum == 0) {
        next->quantum = quantum;
    }

    vmm_switch_page_directory(next->page_dir);
    process_set_current(next);
    return (registers_t*)next->esp;
}

/* Force schedule (voluntary yield) */
void schedule(void) {
    process_t* current = process_get_current();
    if (current != 0) {
        current->quantum = 0;
    }

    __asm__ __volatile__("int $0x20");
}

/* Set quantum */
void scheduler_set_quantum(uint32_t q) {
    if (q > 0) {
        quantum = q;
    }
}

/* Get quantum */
uint32_t scheduler_get_quantum(void) {
    return quantum;
}

/* Get number of ready processes */
uint32_t scheduler_get_ready_count(void) {
    uint32_t count = 0;
    process_t* proc = process_list;

    if (proc == 0) {
        return 0;
    }

    process_t* start = proc;
    do {
        if (proc->state == PROC_STATE_READY || proc->state == PROC_STATE_RUNNING) {
            count++;
        }
        proc = proc->next;
    } while (proc != 0 && proc != start);

    return count;
}
