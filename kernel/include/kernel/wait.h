/* SYNAPSE SO - Wait System Call */
/* Licensed under GPLv3 */

#ifndef KERNEL_WAIT_H
#define KERNEL_WAIT_H

#include <kernel/process.h>

/* Wait system call implementation */
pid_t do_wait(pid_t pid, int* status);

#endif /* KERNEL_WAIT_H */
