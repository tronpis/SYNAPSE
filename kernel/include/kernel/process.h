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
} process_t;

typedef void (*process_entry_t)(void);

/* Process list management */
void process_init(void);
process_t* process_create(const char* name, uint32_t flags,
                          process_entry_t entry);
process_t* process_create_current(const char* name);
void process_destroy(process_t* proc);
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

#endif /* KERNEL_PROCESS_H */
