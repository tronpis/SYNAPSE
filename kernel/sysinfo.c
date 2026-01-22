/* SYNAPSE SO - System Information Implementation */
/* Licensed under GPLv3 */

#include <kernel/sysinfo.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/scheduler.h>
#include <kernel/process.h>
#include <kernel/timer.h>
#include <kernel/vga.h>

/* Version information */
#define SYNAPSE_VERSION "0.3.0-alpha"
#define BUILD_DATE "January 2025"

/* Get system information */
void sysinfo_get(sysinfo_t* info) {
    if (info == 0) {
        return;
    }
    
    /* Get PMM stats */
    pmm_stats_t pmm_stats;
    pmm_get_stats(&pmm_stats);
    
    /* Convert frames to KB */
    info->total_memory_kb = (pmm_stats.total_frames * 4096) / 1024;
    info->free_memory_kb = (pmm_stats.free_frames * 4096) / 1024;
    info->used_memory_kb = (pmm_stats.used_frames * 4096) / 1024;
    info->shared_memory_kb = (pmm_stats.shared_frames * 4096) / 1024;
    
    /* Get scheduler stats */
    scheduler_stats_t sched_stats;
    scheduler_get_stats(&sched_stats);
    
    /* Calculate CPU usage */
    uint32_t total_ticks = sched_stats.busy_ticks + sched_stats.idle_ticks;
    if (total_ticks > 0) {
        info->cpu_usage_percent = (sched_stats.busy_ticks * 100) / total_ticks;
    } else {
        info->cpu_usage_percent = 0;
    }
    
    info->idle_time_ticks = sched_stats.idle_ticks;
    info->busy_time_ticks = sched_stats.busy_ticks;
    info->context_switches = sched_stats.total_switches;
    
    /* Get process counts */
    info->total_processes = 0;
    info->running_processes = 0;
    info->blocked_processes = 0;
    info->zombie_processes = 0;
    
    process_t* proc = process_list;
    if (proc != 0) {
        process_t* start = proc;
        do {
            info->total_processes++;
            switch (proc->state) {
                case PROC_STATE_RUNNING:
                    info->running_processes++;
                    break;
                case PROC_STATE_BLOCKED:
                    info->blocked_processes++;
                    break;
                case PROC_STATE_ZOMBIE:
                    info->zombie_processes++;
                    break;
                default:
                    break;
            }
            proc = proc->next;
        } while (proc != 0 && proc != start);
    }
    
    /* Uptime */
    info->uptime_ticks = timer_get_ticks();
    
    /* Version */
    info->version = SYNAPSE_VERSION;
    info->build_date = BUILD_DATE;
}

/* Print system information */
void sysinfo_print(void) {
    sysinfo_t info;
    sysinfo_get(&info);
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== SYNAPSE SO System Information ===\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("Version: ");
    vga_print(info.version);
    vga_print(" (");
    vga_print(info.build_date);
    vga_print(")\n");
    
    vga_print("Uptime: ");
    vga_print_dec(info.uptime_ticks / 100);
    vga_print(" seconds\n\n");
    
    /* Memory info */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("Memory:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Total:  ");
    vga_print_dec(info.total_memory_kb);
    vga_print(" KB\n");
    vga_print("  Used:   ");
    vga_print_dec(info.used_memory_kb);
    vga_print(" KB\n");
    vga_print("  Free:   ");
    vga_print_dec(info.free_memory_kb);
    vga_print(" KB\n");
    vga_print("  Shared: ");
    vga_print_dec(info.shared_memory_kb);
    vga_print(" KB\n\n");
    
    /* CPU info */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("CPU:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Usage: ");
    vga_print_dec(info.cpu_usage_percent);
    vga_print("%\n");
    vga_print("  Context switches: ");
    vga_print_dec(info.context_switches);
    vga_print("\n\n");
    
    /* Process info */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("Processes:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Total:   ");
    vga_print_dec(info.total_processes);
    vga_print("\n");
    vga_print("  Running: ");
    vga_print_dec(info.running_processes);
    vga_print("\n");
    vga_print("  Blocked: ");
    vga_print_dec(info.blocked_processes);
    vga_print("\n");
    vga_print("  Zombie:  ");
    vga_print_dec(info.zombie_processes);
    vga_print("\n");
}

/* Print memory usage */
void sysinfo_print_memory(void) {
    pmm_stats_t pmm;
    pmm_get_stats(&pmm);
    
    vmm_stats_t vmm;
    vmm_get_stats(&vmm);
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== Memory Statistics ===\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\nPhysical Memory (PMM):\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Total frames:  ");
    vga_print_dec(pmm.total_frames);
    vga_print("\n");
    vga_print("  Used frames:   ");
    vga_print_dec(pmm.used_frames);
    vga_print("\n");
    vga_print("  Free frames:   ");
    vga_print_dec(pmm.free_frames);
    vga_print("\n");
    vga_print("  Shared frames: ");
    vga_print_dec(pmm.shared_frames);
    vga_print("\n");
    
    uint32_t pmm_usage = (pmm.used_frames * 100) / pmm.total_frames;
    vga_print("  Usage: ");
    vga_print_dec(pmm_usage);
    vga_print("%\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\nVirtual Memory (VMM):\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Total pages:  ");
    vga_print_dec(vmm.total_pages);
    vga_print("\n");
    vga_print("  Used pages:   ");
    vga_print_dec(vmm.used_pages);
    vga_print("\n");
    vga_print("  Free pages:   ");
    vga_print_dec(vmm.free_pages);
    vga_print("\n");
    vga_print("  COW pages:    ");
    vga_print_dec(vmm.cow_pages);
    vga_print("\n");
    vga_print("  Shared pages: ");
    vga_print_dec(vmm.shared_pages);
    vga_print("\n");
}

/* Print process list */
void sysinfo_print_processes(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== Process List ===\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("PID  STATE    PRIO NAME\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    process_t* proc = process_list;
    if (proc == 0) {
        vga_print("No processes\n");
        return;
    }
    
    process_t* start = proc;
    do {
        /* PID */
        vga_print_dec(proc->pid);
        vga_print("  ");
        
        /* State */
        switch (proc->state) {
            case PROC_STATE_READY:
                vga_print("READY   ");
                break;
            case PROC_STATE_RUNNING:
                vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
                vga_print("RUNNING ");
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                break;
            case PROC_STATE_BLOCKED:
                vga_print("BLOCKED ");
                break;
            case PROC_STATE_ZOMBIE:
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                vga_print("ZOMBIE  ");
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                break;
            case PROC_STATE_STOPPED:
                vga_print("STOPPED ");
                break;
            default:
                vga_print("UNKNOWN ");
                break;
        }
        
        /* Priority */
        vga_print_dec(proc->priority);
        vga_print("    ");
        
        /* Name */
        vga_print(proc->name);
        vga_print("\n");
        
        proc = proc->next;
    } while (proc != 0 && proc != start);
}

/* Print scheduler stats */
void sysinfo_print_scheduler(void) {
    scheduler_stats_t stats;
    scheduler_get_stats(&stats);
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== Scheduler Statistics ===\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("Context switches: ");
    vga_print_dec(stats.total_switches);
    vga_print("\n");
    
    vga_print("Idle ticks:       ");
    vga_print_dec(stats.idle_ticks);
    vga_print("\n");
    
    vga_print("Busy ticks:       ");
    vga_print_dec(stats.busy_ticks);
    vga_print("\n");
    
    uint32_t total = stats.idle_ticks + stats.busy_ticks;
    if (total > 0) {
        uint32_t cpu_usage = (stats.busy_ticks * 100) / total;
        vga_print("CPU usage:        ");
        vga_print_dec(cpu_usage);
        vga_print("%\n");
    }
    
    vga_print("\nProcesses ready:  ");
    vga_print_dec(stats.processes_ready);
    vga_print("\n");
    
    vga_print("Processes blocked: ");
    vga_print_dec(stats.processes_blocked);
    vga_print("\n");
}
