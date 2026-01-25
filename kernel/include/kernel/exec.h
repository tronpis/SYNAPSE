/* SYNAPSE SO - Exec System Call */
/* Licensed under GPLv3 */

#ifndef KERNEL_EXEC_H
#define KERNEL_EXEC_H

#include <stdint.h>
#include <kernel/const.h>

/* Exec system call implementation */
int do_exec(const char* path, char* const argv[]);

/* Destroy a page directory and free its pages */
void vmm_destroy_page_directory(void* pd);

#endif /* KERNEL_EXEC_H */
