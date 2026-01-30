/* SYNAPSE SO - Exec System Call */
/* Licensed under GPLv3 */

#ifndef KERNEL_EXEC_H
#define KERNEL_EXEC_H

#include <stdint.h>
#include <kernel/const.h>
#include <kernel/vmm.h>

/* Exec system call implementation */
int do_exec(const char* path, char* const argv[]);

#endif /* KERNEL_EXEC_H */
