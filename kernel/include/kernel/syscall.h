/* SYNAPSE SO - System Call Interface */
/* Licensed under GPLv3 */

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>

#include <kernel/idt.h>

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
#define SYS_LSEEK    10
#define SYS_GETPPID  11
#define SYS_YIELD    12
#define SYS_SLEEP    13
#define SYS_BRK      14
#define SYS_SBRK     15
#define SYS_STAT     16
#define SYS_FSTAT    17
#define SYS_KILL     18
#define SYS_SIGNAL   19
#define SYS_UPTIME   20
#define SYS_SYSINFO  21
#define SYS_GETUID   22
#define SYS_SETUID   23
#define SYS_DUP      24
#define SYS_PIPE     25
#define SYS_CHDIR    26
#define SYS_GETCWD   27
#define SYS_MKDIR    28
#define SYS_RMDIR    29
#define SYS_UNLINK   30
#define SYS_GETTIME  31
#define SYS_REBOOT   32

/* Maximum number of system calls */
#define NUM_SYSCALLS 64

/* System call function prototype */
typedef int (*syscall_func_t)(uint32_t arg1, uint32_t arg2,
                              uint32_t arg3, uint32_t arg4,
                              uint32_t arg5);

/* Initialize system call interface */
void syscall_init(void);

/* Register a system call handler */
void syscall_register(uint32_t num, syscall_func_t handler);

/* System call handler (called from assembly) */
registers_t* syscall_handler(registers_t* regs);

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
int sys_lseek(int fd, int offset, int whence);
int sys_getppid(void);
int sys_yield(void);
int sys_sleep(uint32_t ticks);
int sys_brk(uint32_t addr);
int sys_sbrk(int32_t increment);
int sys_stat(uint32_t path, uint32_t statbuf);
int sys_fstat(uint32_t fd, uint32_t statbuf);
int sys_kill(int32_t pid, int32_t sig);
int sys_signal(int32_t signum, uint32_t handler);
int sys_uptime(void);
int sys_sysinfo(uint32_t info);
int sys_getuid(void);
int sys_setuid(uint32_t uid);
int sys_dup(uint32_t oldfd);
int sys_pipe(uint32_t pipefd);
int sys_chdir(uint32_t path);
int sys_getcwd(uint32_t buf, uint32_t size);
int sys_mkdir(uint32_t path, uint32_t mode);
int sys_rmdir(uint32_t path);
int sys_unlink(uint32_t path);
int sys_gettime(uint32_t timeval);
int sys_reboot(uint32_t cmd);

uint32_t syscall_get_num(registers_t* regs);
void syscall_set_return(registers_t* regs, uint32_t value);

/* Signal numbers */
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1    10
#define SIGSEGV    11
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
#define SIGTERM    15
#define SIGCHLD    17
#define SIGCONT    18
#define SIGSTOP    19
#define SIGTSTP    20

/* Stat structure */
typedef struct {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
} stat_t;

/* Sysinfo structure */
typedef struct {
    uint32_t uptime;
    uint32_t total_mem;
    uint32_t free_mem;
    uint32_t total_processes;
    uint32_t running_processes;
    uint32_t context_switches;
} sysinfo_t;

/* Reboot commands */
#define REBOOT_CMD_RESTART  0x01234567
#define REBOOT_CMD_HALT     0xDEADBEEF
#define REBOOT_CMD_POWEROFF 0x87654321

#endif /* KERNEL_SYSCALL_H */
