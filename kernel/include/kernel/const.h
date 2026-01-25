/* SYNAPSE SO - Kernel Constants */
/* Licensed under GPLv3 */

#ifndef KERNEL_CONST_H
#define KERNEL_CONST_H

/* Stack sizes */
#define KERNEL_STACK_SIZE 0x2000
#define USER_STACK_SIZE   0x1000

/* Process states */
#define PROC_STATE_READY    0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_BLOCKED  2
#define PROC_STATE_ZOMBIE   3
#define PROC_STATE_STOPPED  4

/* Process flags */
#define PROC_FLAG_KERNEL    (1 << 0)

#endif /* KERNEL_CONST_H */
