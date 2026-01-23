/* SYNAPSE SO - Scheduler Priority Support Implementation */
/* Licensed under GPLv3 */

#include <kernel/scheduler.h>
#include <kernel/process.h>
#include <kernel/vga.h>

/* Scheduler statistics */
static scheduler_stats_t sched_stats = {0};

/* Set process priority */
void scheduler_set_priority(process_t* proc, uint32_t priority) {
    if (proc == 0) {
        return;
    }
    
    if (priority > PRIORITY_MAX) {
        priority = PRIORITY_MAX;
    }
    
    proc->priority = priority;
    vga_print("[+] Set priority ");
    vga_print_dec(priority);
    vga_print(" for process ");
    vga_print(proc->name);
    vga_print("\n");
}

/* Get process priority */
uint32_t scheduler_get_priority(process_t* proc) {
    if (proc == 0) {
        return PRIORITY_NORMAL;
    }
    
    return proc->priority;
}

/* Boost priority temporarily */
void scheduler_boost_priority(process_t* proc) {
    if (proc == 0) {
        return;
    }
    
    /* Increase priority by 1 level, but don't exceed maximum */
    if (proc->priority < PRIORITY_MAX) {
        proc->priority++;
        vga_print("[+] Boosted priority for process ");
        vga_print(proc->name);
        vga_print(" to ");
        vga_print_dec(proc->priority);
        vga_print("\n");
    }
}

/* Count blocked processes */
static uint32_t count_blocked_processes(void) {
    uint32_t count = 0;
    process_t* proc = process_list;
    
    if (proc == 0) {
        return 0;
    }
    
    process_t* start = proc;
    do {
        if (proc->state == PROC_STATE_BLOCKED) {
            count++;
        }
        proc = proc->next;
    } while (proc != 0 && proc != start);
    
    return count;
}

/* Get scheduler statistics */
void scheduler_get_stats(scheduler_stats_t* stats) {
    if (stats == 0) {
        return;
    }
    
    *stats = sched_stats;
    
    /* Update process counts */
    stats->processes_ready = scheduler_get_ready_count();
    stats->processes_blocked = count_blocked_processes();
}

/* Reset scheduler statistics */
void scheduler_reset_stats(void) {
    sched_stats.total_switches = 0;
    sched_stats.idle_ticks = 0;
    sched_stats.busy_ticks = 0;
    sched_stats.processes_ready = 0;
    sched_stats.processes_blocked = 0;
}

/* Update statistics (called from scheduler_tick) */
void scheduler_update_stats(int was_idle) {
    if (was_idle) {
        sched_stats.idle_ticks++;
    } else {
        sched_stats.busy_ticks++;
    }
}

/* Increment context switch counter */
void scheduler_count_switch(void) {
    sched_stats.total_switches++;
}
