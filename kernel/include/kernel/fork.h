/* SYNAPSE SO - Fork System Call */
/* Licensed under GPLv3 */

#ifndef KERNEL_FORK_H
#define KERNEL_FORK_H

#include <kernel/process.h>

/* Next PID counter (extern for fork.c) */
extern uint32_t next_pid;

/* Fork system call implementation */
pid_t do_fork(void);

/* Vfork system call (simplified) */
pid_t sys_vfork(void);

#endif /* KERNEL_FORK_H */
