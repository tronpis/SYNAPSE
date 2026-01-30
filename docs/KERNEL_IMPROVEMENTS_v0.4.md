# SYNAPSE SO - Kernel Improvements v0.4.0

## Overview

This document describes the kernel improvements implemented in version 0.4.0. These changes significantly enhance the kernel's functionality, adding 22 new system calls and improving process management capabilities.

## New System Calls

### Process Management
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_getppid` | 11 | Get parent process ID |
| `sys_yield` | 12 | Voluntarily yield CPU to other processes |
| `sys_sleep` | 13 | Sleep for specified number of ticks |

### Memory Management
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_brk` | 14 | Set program break (heap top) |
| `sys_sbrk` | 15 | Increment/decrement program break |

### File System
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_stat` | 16 | Get file statistics by path |
| `sys_fstat` | 17 | Get file statistics by file descriptor |
| `sys_dup` | 24 | Duplicate file descriptor (stub) |
| `sys_pipe` | 25 | Create pipe (stub) |
| `sys_chdir` | 26 | Change working directory |
| `sys_getcwd` | 27 | Get current working directory |
| `sys_mkdir` | 28 | Create directory (stub) |
| `sys_rmdir` | 29 | Remove directory (stub) |
| `sys_unlink` | 30 | Delete file (stub) |

### Signal Handling
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_kill` | 18 | Send signal to process |
| `sys_signal` | 19 | Set signal handler |

### System Information
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_uptime` | 20 | Get system uptime in seconds |
| `sys_sysinfo` | 21 | Get comprehensive system information |
| `sys_gettime` | 31 | Get current time (uptime-based) |

### User Management
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_getuid` | 22 | Get user ID |
| `sys_setuid` | 23 | Set user ID (root only) |

### System Control
| Syscall | Number | Description |
|---------|--------|-------------|
| `sys_reboot` | 32 | Reboot, halt, or power off system |

## Signal Handling Implementation

### Supported Signals
- SIGHUP (1) - Hangup
- SIGINT (2) - Interrupt
- SIGQUIT (3) - Quit
- SIGILL (4) - Illegal instruction
- SIGTRAP (5) - Trace trap
- SIGABRT (6) - Abort
- SIGBUS (7) - Bus error
- SIGFPE (8) - Floating point exception
- SIGKILL (9) - Kill (cannot be caught)
- SIGUSR1 (10) - User signal 1
- SIGSEGV (11) - Segmentation fault
- SIGUSR2 (12) - User signal 2
- SIGPIPE (13) - Broken pipe
- SIGALRM (14) - Alarm
- SIGTERM (15) - Termination
- SIGCHLD (17) - Child status change
- SIGCONT (18) - Continue
- SIGSTOP (19) - Stop (cannot be caught)
- SIGTSTP (20) - Terminal stop

### Signal Features
- Per-process signal handlers
- Pending signal tracking
- Default signal actions (terminate, ignore, stop)
- SIGKILL and SIGSTOP cannot be caught or ignored
- Signal delivery wakes blocked processes

## Process Enhancements

### New PCB Fields
```c
struct process {
    // ... existing fields ...
    uint32_t brk;           // Program break (heap top)
    uint32_t uid;           // User ID
    uint32_t gid;           // Group ID
    uint32_t euid;          // Effective user ID
    uint32_t egid;          // Effective group ID
    uint32_t pending_signals;
    signal_handler_t signal_handlers[NSIG];
    char cwd[256];          // Current working directory
    uint32_t wake_tick;     // Sleep wakeup tick
};
```

### Process Functions
- `process_send_signal()` - Send signal to process
- `process_set_signal_handler()` - Set signal handler
- `process_check_signals()` - Check and handle pending signals
- `process_get_uid()` / `process_set_uid()` - UID management
- `process_get_gid()` / `process_set_gid()` - GID management
- `process_brk()` / `process_sbrk()` - Program break management
- `process_get_cwd()` / `process_set_cwd()` - Working directory
- `process_sleep_until()` - Schedule process wake
- `process_check_sleeping()` - Wake sleeping processes
- `process_count_total()` - Count all processes
- `process_count_running()` - Count running/ready processes

## Timer Improvements

### New Functions
- `timer_get_frequency()` - Get timer frequency in Hz
- `timer_sleep()` - Simple blocking sleep
- `timer_get_uptime_seconds()` - Get uptime in seconds
- `timer_get_uptime_ms()` - Get uptime in milliseconds

## Scheduler Improvements

- Integrated sleep/wake management in scheduler_tick()
- Signal checking on each tick
- Wake sleeping processes automatically

## Shell Improvements (v0.4)

### New Commands
| Command | Description |
|---------|-------------|
| `uptime` | Show system uptime |
| `mem` | Show memory usage |
| `sysinfo` | Show comprehensive system information |
| `kill <pid>` | Send SIGTERM to process |
| `sleep <n>` | Sleep for n ticks |
| `pwd` | Print working directory |
| `cd <path>` | Change directory |
| `reboot` | Reboot system |
| `halt` | Halt system |

## Data Structures

### stat_t Structure
```c
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
```

### sysinfo_t Structure
```c
typedef struct {
    uint32_t uptime;
    uint32_t total_mem;
    uint32_t free_mem;
    uint32_t total_processes;
    uint32_t running_processes;
    uint32_t context_switches;
} sysinfo_t;
```

## Future Work (Stubs)

The following syscalls are stubbed for future implementation:
- `sys_dup` - Requires per-process file descriptor table
- `sys_pipe` - Requires IPC infrastructure
- `sys_mkdir` - Requires VFS directory support
- `sys_rmdir` - Requires VFS directory support
- `sys_unlink` - Requires VFS file deletion

## Files Modified

### Headers
- `kernel/include/kernel/syscall.h` - New syscall definitions
- `kernel/include/kernel/timer.h` - New timer functions
- `kernel/include/kernel/process.h` - New process fields and functions
- `kernel/include/kernel/pmm.h` - Added pmm_get_total_frames()

### Source Files
- `kernel/syscall.c` - New syscall implementations
- `kernel/timer.c` - New timer functions
- `kernel/process.c` - Signal handling, UID/GID, brk/sbrk, cwd, sleep
- `kernel/scheduler.c` - Integrated sleep/signal checking
- `kernel/pmm.c` - Added pmm_get_total_frames()
- `kernel/kernel.c` - New shell commands, version bump

## Version History

- **v0.4.0** - Added 22 new system calls, signal handling, improved shell
- **v0.3.0** - VFS, fork, exec, wait, basic syscalls
- **v0.2.0** - Memory management, process scheduling
- **v0.1.0** - Boot, GDT, IDT, VGA driver

---

*Document created: January 2025*
*Version: 0.4.0*
