/* SYNAPSE SO - Scheduler */
/* Licensed under GPLv3 */

#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <stdint.h>
#include <kernel/idt.h>
#include <kernel/process.h>

/* Scheduler quantum (time slice) */
#define DEFAULT_QUANTUM 10

/* Process priorities */
#define PRIORITY_IDLE       0   /* Idle process */
#define PRIORITY_LOW        1   /* Low priority */
#define PRIORITY_NORMAL     2   /* Normal priority (default) */
#define PRIORITY_HIGH       3   /* High priority */
#define PRIORITY_REALTIME   4   /* Real-time priority */
#define PRIORITY_MAX        4   /* Maximum priority */

/* Initialize scheduler */
void scheduler_init(void);

/* Add process to scheduler */
void scheduler_add_process(process_t* proc);

/* Remove process from scheduler */
void scheduler_remove_process(process_t* proc);

/* Schedule next process (called by timer interrupt)
 * Returns the register frame to restore (for context switching).
 */
/* Must be called with a valid register frame pointer from the timer ISR; passing NULL is undefined. */
registers_t* scheduler_tick(registers_t* regs) __attribute__((nonnull(1)));

/* Force schedule */
void schedule(void);

/* Set quantum */
void scheduler_set_quantum(uint32_t quantum);

/* Get quantum */
uint32_t scheduler_get_quantum(void);

/* Get number of ready processes */
uint32_t scheduler_get_ready_count(void);

/* Context switch function (assembly) */
void context_switch(process_t* old_proc, process_t* new_proc);

/* Initialize context for new process */
void context_init(process_t* proc, uint32_t entry_point);

/* Priority-based scheduling */
/* Set process priority */
void scheduler_set_priority(process_t* proc, uint32_t priority);

/* Get process priority */
uint32_t scheduler_get_priority(process_t* proc);

/* Boost priority temporarily (for I/O bound processes) */
void scheduler_boost_priority(process_t* proc);

/* Scheduler statistics */
typedef struct {
    uint32_t total_switches;
    uint32_t idle_ticks;
    uint32_t busy_ticks;
    uint32_t processes_ready;
    uint32_t processes_blocked;
} scheduler_stats_t;

/* Get scheduler statistics */
void scheduler_get_stats(scheduler_stats_t* stats);

/* Reset scheduler statistics */
void scheduler_reset_stats(void);

/* Internal helpers implemented in scheduler_priority.c (used by scheduler.c) */
void scheduler_update_stats(int was_idle);
void scheduler_count_switch(void);

#endif /* KERNEL_SCHEDULER_H */
