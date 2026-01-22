/* SYNAPSE SO - Scheduler Priority Support (Stub Implementation) */
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
    
    /* TODO: Full implementation
     * 1. Increase priority by 1 level
     * 2. Mark as boosted
     * 3. Reset after some time
     */
    
    if (proc->priority < PRIORITY_MAX) {
        proc->priority++;
    }
}

/* Get scheduler statistics */
void scheduler_get_stats(scheduler_stats_t* stats) {
    if (stats == 0) {
        return;
    }
    
    *stats = sched_stats;
    
    /* Update process counts */
    stats->processes_ready = scheduler_get_ready_count();
    stats->processes_blocked = 0;  /* TODO: Count blocked processes */
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
