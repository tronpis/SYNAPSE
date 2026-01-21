/* SYNAPSE SO - System Call Interface */
/* Licensed under GPLv3 */

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>

/* System call numbers */
#define SYS_EXIT     1
#define SYS_WRITE    2
#define SYS_READ     3
#define SYS_OPEN     4
#define SYS_CLOSE    5
#define SYS_FORK     6
#define SYS_EXEC     7
#define SYS_WAIT     8
#define SYS_GETPID   9

/* Maximum number of system calls */
#define NUM_SYSCALLS 32

/* System call function prototype */
typedef int (*syscall_func_t)(uint32_t arg1, uint32_t arg2,
                              uint32_t arg3, uint32_t arg4,
                              uint32_t arg5);

/* Initialize system call interface */
void syscall_init(void);

/* Register a system call handler */
void syscall_register(uint32_t num, syscall_func_t handler);

/* System call handler (called from assembly) */
void syscall_handler(registers_t* regs);

/* Individual system call implementations */
int sys_exit(uint32_t exit_code);
int sys_write(uint32_t fd, uint32_t buffer, uint32_t count);
int sys_read(uint32_t fd, uint32_t buffer, uint32_t count);
int sys_open(uint32_t filename, uint32_t flags, uint32_t mode);
int sys_close(uint32_t fd);
int sys_fork(void);
int sys_exec(uint32_t path, uint32_t argv);
int sys_wait(uint32_t pid, uint32_t status);
int sys_getpid(void);

/* Helper to get syscall number from registers */
static inline uint32_t syscall_get_num(registers_t* regs) {
    return regs->eax;
}

/* Helper to set return value */
static inline void syscall_set_return(registers_t* regs, uint32_t value) {
    regs->eax = value;
}

#endif /* KERNEL_SYSCALL_H */
