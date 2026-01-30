/* SYNAPSE SO - Process Management */
/* Licensed under GPLv3 */

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <kernel/vmm.h>
#include <kernel/const.h>

/* Process ID */
typedef uint32_t pid_t;

/* Forward declaration for process list */
typedef struct process process_t;

/* Global process list (extern for scheduler access) */
extern process_t* process_list;

/* Signal handler type */
typedef void (*signal_handler_t)(int);

/* Default signal handlers */
#define SIG_DFL ((signal_handler_t)0)
#define SIG_IGN ((signal_handler_t)1)
#define SIG_ERR ((signal_handler_t)-1)

/* Maximum number of signals */
#define NSIG 32

/* Process Control Block */
typedef struct process {
    /* Process identification */
    pid_t pid;
    pid_t ppid;
    char name[32];
    uint32_t state;
    uint32_t flags;

    /* Memory management */
    page_directory_t* page_dir;
    uint32_t heap_start;
    uint32_t heap_end;
    uint32_t stack_start;
    uint32_t stack_end;
    uint32_t brk;           /* Program break (heap top) */

    /* CPU context */
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t eflags;

    /* Registers */
    uint32_t eax, ebx, ecx, edx, esi, edi;

    /* Process list */
    struct process* next;
    struct process* prev;

    /* Exit status */
    uint32_t exit_code;

    /* Priority (0 = lowest, 31 = highest) */
    uint32_t priority;

    /* Time quantum remaining */
    uint32_t quantum;

    /* User/Group IDs */
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;

    /* Signal handling */
    uint32_t pending_signals;
    signal_handler_t signal_handlers[NSIG];

    /* Current working directory */
    char cwd[256];

    /* Sleep/wake management */
    uint32_t wake_tick;     /* Tick at which process should wake */
} process_t;

typedef void (*process_entry_t)(void);

/* Process list management */
void process_init(void);
process_t* process_create(const char* name, uint32_t flags,
                          process_entry_t entry);
process_t* process_create_current(const char* name);
void process_destroy(process_t* proc);

/* Insert a process into the global process list. */
void process_add_to_list(process_t* proc);

process_t* process_get_current(void);
void process_set_current(process_t* proc);
process_t* process_get_list(void);
process_t* process_find_by_pid(pid_t pid);

/* Process state management */
void process_set_state(process_t* proc, uint32_t state);
void process_ready(process_t* proc);
void process_block(process_t* proc);
void process_unblock(process_t* proc);

/* Process scheduling */
void schedule(void);
void scheduler_init(void);
void scheduler_add_process(process_t* proc);
void scheduler_remove_process(process_t* proc);

/* Process execution */
int process_exec(uint8_t* elf_data, uint32_t size);
void process_exit(int exit_code);

/* Process utilities */
pid_t process_get_pid(void);
pid_t process_get_ppid(void);
void process_set_name(process_t* proc, const char* name);

/* Idle process */
void idle_process(void);

/* Signal handling */
int process_send_signal(process_t* proc, int signum);
signal_handler_t process_set_signal_handler(process_t* proc, int signum, 
                                            signal_handler_t handler);
void process_check_signals(process_t* proc);

/* User/Group ID management */
uint32_t process_get_uid(process_t* proc);
void process_set_uid(process_t* proc, uint32_t uid);
uint32_t process_get_gid(process_t* proc);
void process_set_gid(process_t* proc, uint32_t gid);

/* Program break (brk/sbrk) */
uint32_t process_brk(process_t* proc, uint32_t addr);
int32_t process_sbrk(process_t* proc, int32_t increment);

/* Working directory */
const char* process_get_cwd(process_t* proc);
int process_set_cwd(process_t* proc, const char* path);

/* Sleep management */
void process_sleep_until(process_t* proc, uint32_t wake_tick);
void process_check_sleeping(void);

/* Process counting */
uint32_t process_count_total(void);
uint32_t process_count_running(void);

#endif /* KERNEL_PROCESS_H */
