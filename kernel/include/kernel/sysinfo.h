/* SYNAPSE SO - System Information */
/* Licensed under GPLv3 */

#ifndef KERNEL_SYSINFO_H
#define KERNEL_SYSINFO_H

#include <stdint.h>

/* System information structure */
typedef struct {
    /* Memory info */
    uint32_t total_memory_kb;
    uint32_t free_memory_kb;
    uint32_t used_memory_kb;
    uint32_t shared_memory_kb;
    
    /* CPU info */
    uint32_t cpu_usage_percent;
    uint32_t idle_time_ticks;
    uint32_t busy_time_ticks;
    
    /* Process info */
    uint32_t total_processes;
    uint32_t running_processes;
    uint32_t blocked_processes;
    uint32_t zombie_processes;
    
    /* Scheduler info */
    uint32_t context_switches;
    uint32_t uptime_ticks;
    
    /* Version info */
    const char* version;
    const char* build_date;
} sysinfo_t;

/* Get system information */
void sysinfo_get(sysinfo_t* info);

/* Print system information to VGA */
void sysinfo_print(void);

/* Print memory usage */
void sysinfo_print_memory(void);

/* Print process list */
void sysinfo_print_processes(void);

/* Print scheduler stats */
void sysinfo_print_scheduler(void);

#endif /* KERNEL_SYSINFO_H */
