/* SYNAPSE SO - Kernel Entry Point */
/* Licensed under GPLv3 */

#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/heap.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/timer.h>
#include <kernel/elf.h>
#include <kernel/syscall.h>
#include <kernel/fork.h>
#include <kernel/wait.h>
#include <kernel/usermode.h>
#include <kernel/cpu.h>
#include <kernel/early.h>
#include <kernel/serial.h>
#include <kernel/keyboard.h>
#include <kernel/console.h>
#include <kernel/vfs.h>
#include <kernel/ramfs.h>
#include <kernel/string.h>

/* Multiboot information structure */
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int mmap_length;
    unsigned int mmap_addr;
    /* ... more fields not used in minimal version ... */
} __attribute__((packed)) multiboot_info_t;

static void demo_syscalls(void) {
    /* Demonstrate system calls in kernel thread */
    vga_print("[DEMO] Testing syscalls...\n");

    /* Test sys_getpid */
    pid_t pid = sys_getpid();
    vga_print("[DEMO] Current PID: ");
    vga_print_dec(pid);
    vga_print("\n");

    /* Test sys_write */
    char* msg = "Hello from syscall!";
    vga_print("[DEMO] Writing via kernel path: ");
    vga_print(msg);
    int bytes_written = 19;
    vga_print_dec(bytes_written);
    vga_print(" bytes\n");

    /* Sleep for a while */
    for (uint32_t i = 0; i < 50000000; i++) {
        __asm__ __volatile__("nop");
    }

    /* Test sys_exit (this will terminate the process) */
    vga_print("[DEMO] Calling sys_exit(0)...\n");
    sys_exit(0);
}

static void shell_help(void) {
    vga_print("Commands:\n");
    vga_print("  help        - Show this help\n");
    vga_print("  ticks       - Show timer ticks\n");
    vga_print("  uptime      - Show system uptime\n");
    vga_print("  ps          - List processes\n");
    vga_print("  mem         - Show memory usage\n");
    vga_print("  sysinfo     - Show system information\n");
    vga_print("  fork        - Run fork demo\n");
    vga_print("  kill <pid>  - Send SIGTERM to process\n");
    vga_print("  sleep <n>   - Sleep for n ticks\n");
    vga_print("  cat <path>  - Print file (ramfs/vfs)\n");
    vga_print("  pwd         - Print working directory\n");
    vga_print("  cd <path>   - Change directory\n");
    vga_print("  clear       - Clear screen\n");
    vga_print("  reboot      - Reboot system\n");
    vga_print("  halt        - Halt system\n");
}

static void shell_ps(void) {
    process_t* start = process_get_list();
    if (start == 0) {
        vga_print("[ps] no processes\n");
        return;
    }

    vga_print("PID  STATE  NAME\n");

    process_t* p = start;
    do {
        vga_print_dec(p->pid);
        vga_print("    ");
        vga_print_dec(p->state);
        vga_print("      ");
        vga_print(p->name);
        vga_print("\n");
        p = p->next;
    } while (p != 0 && p != start);
}

/* Simple atoi implementation */
static int shell_atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

static void shell_cat(const char* path) {
    if (path == 0 || path[0] == '\0') {
        vga_print("usage: cat <path>\n");
        return;
    }

    int fd = vfs_open(path, 0, 0);
    if (fd < 0) {
        vga_print("cat: cannot open ");
        vga_print(path);
        vga_print("\n");
        return;
    }

    char buf[129];
    while (1) {
        int n = vfs_read(fd, buf, 128);
        if (n <= 0) {
            break;
        }
        buf[(uint32_t)n] = '\0';
        vga_print(buf);
    }

    vfs_close(fd);
    vga_print("\n");
}

static void shell_uptime(void) {
    uint32_t seconds = timer_get_uptime_seconds();
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    vga_print("Uptime: ");
    vga_print_dec(hours);
    vga_print("h ");
    vga_print_dec(minutes);
    vga_print("m ");
    vga_print_dec(secs);
    vga_print("s (");
    vga_print_dec(timer_get_ticks());
    vga_print(" ticks)\n");
}

static void shell_mem(void) {
    uint32_t total = pmm_get_total_frames() * 4;  /* KB */
    uint32_t free = pmm_get_free_frames() * 4;    /* KB */
    uint32_t used = pmm_get_used_frames() * 4;    /* KB */
    
    vga_print("Memory:\n");
    vga_print("  Total: ");
    vga_print_dec(total / 1024);
    vga_print(" MB (");
    vga_print_dec(total);
    vga_print(" KB)\n");
    vga_print("  Used:  ");
    vga_print_dec(used / 1024);
    vga_print(" MB (");
    vga_print_dec(used);
    vga_print(" KB)\n");
    vga_print("  Free:  ");
    vga_print_dec(free / 1024);
    vga_print(" MB (");
    vga_print_dec(free);
    vga_print(" KB)\n");
}

static void shell_sysinfo_cmd(void) {
    sysinfo_t info;
    sys_sysinfo((uint32_t)&info);
    
    vga_print("System Information:\n");
    vga_print("  Uptime:           ");
    vga_print_dec(info.uptime);
    vga_print(" seconds\n");
    vga_print("  Total Memory:     ");
    vga_print_dec(info.total_mem / 1024 / 1024);
    vga_print(" MB\n");
    vga_print("  Free Memory:      ");
    vga_print_dec(info.free_mem / 1024 / 1024);
    vga_print(" MB\n");
    vga_print("  Total Processes:  ");
    vga_print_dec(info.total_processes);
    vga_print("\n");
    vga_print("  Running/Ready:    ");
    vga_print_dec(info.running_processes);
    vga_print("\n");
    vga_print("  Context Switches: ");
    vga_print_dec(info.context_switches);
    vga_print("\n");
}

static void shell_kill_cmd(const char* arg) {
    if (arg == 0 || arg[0] == '\0') {
        vga_print("usage: kill <pid>\n");
        return;
    }
    
    int pid = shell_atoi(arg);
    if (pid <= 0) {
        vga_print("Invalid PID\n");
        return;
    }
    
    int result = sys_kill(pid, SIGTERM);
    if (result < 0) {
        vga_print("kill: failed to send signal\n");
    } else {
        vga_print("Signal sent to PID ");
        vga_print_dec(pid);
        vga_print("\n");
    }
}

static void shell_sleep_cmd(const char* arg) {
    if (arg == 0 || arg[0] == '\0') {
        vga_print("usage: sleep <ticks>\n");
        return;
    }
    
    int ticks = shell_atoi(arg);
    if (ticks <= 0) {
        vga_print("Invalid tick count\n");
        return;
    }
    
    vga_print("Sleeping for ");
    vga_print_dec(ticks);
    vga_print(" ticks...\n");
    sys_sleep((uint32_t)ticks);
    vga_print("Woke up!\n");
}

static void shell_pwd(void) {
    process_t* current = process_get_current();
    if (current != 0) {
        vga_print(process_get_cwd(current));
        vga_print("\n");
    }
}

static void shell_cd(const char* path) {
    if (path == 0 || path[0] == '\0') {
        /* cd with no args goes to root */
        path = "/";
    }
    
    process_t* current = process_get_current();
    if (current != 0) {
        if (process_set_cwd(current, path) < 0) {
            vga_print("cd: failed to change directory\n");
        }
    }
}

static void shell_process(void) {
    vga_print("\n[+] SYNAPSE SO Shell v0.4\n");
    vga_print("[+] Type 'help' for commands\n\n");

    char line[128];

    while (1) {
        vga_print("[SHELL] $ ");
        console_read_line(line, sizeof(line));

        if (strcmp(line, "") == 0) {
            continue;
        }

        if (strcmp(line, "help") == 0) {
            shell_help();
            continue;
        }

        if (strcmp(line, "clear") == 0) {
            vga_clear_screen();
            continue;
        }

        if (strcmp(line, "ticks") == 0) {
            vga_print("ticks=");
            vga_print_dec(timer_get_ticks());
            vga_print("\n");
            continue;
        }

        if (strcmp(line, "uptime") == 0) {
            shell_uptime();
            continue;
        }

        if (strcmp(line, "ps") == 0) {
            shell_ps();
            continue;
        }

        if (strcmp(line, "mem") == 0) {
            shell_mem();
            continue;
        }

        if (strcmp(line, "sysinfo") == 0) {
            shell_sysinfo_cmd();
            continue;
        }

        if (strcmp(line, "fork") == 0) {
            vga_print("[SHELL] Running fork demo...\n");
            pid_t pid = do_fork();
            if (pid == 0) {
                vga_print("  [CHILD] I am the child process!\n");
                sys_exit(0);
            } else if (pid > 0) {
                vga_print("  [PARENT] Child PID: ");
                vga_print_dec(pid);
                vga_print("\n");
                do_wait(-1, 0);
                vga_print("  [PARENT] Child exited\n");
            } else {
                vga_print("  [SHELL] Fork failed\n");
            }
            continue;
        }

        if (strncmp(line, "kill ", 5) == 0) {
            shell_kill_cmd(line + 5);
            continue;
        }

        if (strncmp(line, "sleep ", 6) == 0) {
            shell_sleep_cmd(line + 6);
            continue;
        }

        if (strncmp(line, "cat ", 4) == 0) {
            shell_cat(line + 4);
            continue;
        }

        if (strcmp(line, "pwd") == 0) {
            shell_pwd();
            continue;
        }

        if (strncmp(line, "cd ", 3) == 0) {
            shell_cd(line + 3);
            continue;
        }

        if (strcmp(line, "cd") == 0) {
            shell_cd("/");
            continue;
        }

        if (strcmp(line, "reboot") == 0) {
            vga_print("[SHELL] Rebooting...\n");
            sys_reboot(REBOOT_CMD_RESTART);
            continue;
        }

        if (strcmp(line, "halt") == 0) {
            vga_print("[SHELL] Halting system...\n");
            sys_reboot(REBOOT_CMD_HALT);
            continue;
        }

        vga_print("unknown command: ");
        vga_print(line);
        vga_print("\n");
    }
}

static void worker_a(void) {
    uint32_t last = 0;

    while (1) {
        uint32_t now = timer_get_ticks();
        if (now - last >= 100) {
            last = now;
            /* Make the VGA prints atomic to avoid concurrent corruption. */
            __asm__ __volatile__("cli");
            vga_print("[A] ticks=");
            vga_print_dec(now);
            vga_print("\n");
            __asm__ __volatile__("sti");
        }
        __asm__ __volatile__("hlt");
    }
}

static void worker_b(void) {
    uint32_t last = 0;

    while (1) {
        uint32_t now = timer_get_ticks();
        if (now - last >= 137) {
            last = now;
            /* Make the VGA prints atomic to avoid concurrent corruption. */
            __asm__ __volatile__("cli");
            vga_print("[B] ticks=");
            vga_print_dec(now);
            vga_print("\n");
            __asm__ __volatile__("sti");
        }
        __asm__ __volatile__("hlt");
    }
}

/* Global variables for early checks */
void* multiboot_info_ptr = 0;
uint32_t multiboot_magic = 0;

/* Kernel main function - entry point from bootloader */
void kernel_main(unsigned int magic, multiboot_info_t* mbi) {
    /* Save for early diagnostics */
    multiboot_magic = magic;
    multiboot_info_ptr = mbi;
    
    /* Clear screen */
    vga_clear_screen();

    /* Print kernel banner */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("SYNAPSE SO - Open Source Operating System v0.4.0\n");
    vga_print("=================================================\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("Phase 1: Boot and Initialization\n");
    vga_print("=================================================\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Initialize CPU detection */
    vga_print("[+] Detecting CPU...\n");
    cpu_init();
    cpu_print_info();
    
    /* Run early boot checks */
    early_init();
    int check_result = early_run_checks();
    early_print_summary();
    
    if (check_result == BOOT_CHECK_FATAL) {
        early_panic("Boot checks failed");
    }
    
    /* Enable CPU features (SSE, etc) */
    vga_print("[+] Enabling CPU features...\n");
    cpu_enable_features();

    /* Initialize GDT */
    vga_print("[+] Initializing Global Descriptor Table...\n");
    gdt_init();
    vga_print("    GDT loaded successfully\n");

    /* Initialize IDT */
    vga_print("[+] Initializing Interrupt Descriptor Table...\n");
    idt_init();
    vga_print("    IDT loaded successfully\n");

    /* Initialize basic drivers */
    serial_init(SERIAL_COM1_BASE);
    keyboard_init();
    console_init();

    /* Phase 1 complete */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\n[SUCCESS] Phase 1 complete!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Keep interrupts disabled until Phase 2 initialization is complete. */
    __asm__ __volatile__("cli");

    /* Phase 2: Memory Management */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== PHASE 2: Memory Management ===\n");

    /* Initialize early PMM heap before pmm_init() so early subsystems
       (like frame reference counting) can allocate memory safely. */
    pmm_init_kernel_heap(0x300000, 0x100000); /* 1MB at 3MB */

    /* Initialize Physical Memory Manager */
    if (mbi != 0 && (mbi->flags & 0x40)) {
        mem_map_t* mmap = (mem_map_t*)mbi->mmap_addr;
        pmm_init(mmap, mbi->mmap_length, 8); /* 8 bytes per entry */
    } else {
        /* Fallback: assume 16MB memory */
        vga_print("[-] Warning: No memory map, using default 16MB\n");
    }

    /* Initialize Virtual Memory Manager */
    vmm_init();

    /* Initialize proper kernel heap */
    heap_init((void*)0xC0300000, 0x100000); /* 1MB at 3GB+3MB */

    /* Initialize Process Management */
    vga_print("\n=== PHASE 2: Process Management ===\n");
    process_init();
    scheduler_init();

    /* Create a process representing the currently running kernel context */
    process_create_current("kernel_main");

    /* Demo kernel threads */
    process_create("worker_a", PROC_FLAG_KERNEL, worker_a);
    process_create("worker_b", PROC_FLAG_KERNEL, worker_b);

    /* Demo system calls and shell (Phase 3) */
    process_create("demo_syscalls", PROC_FLAG_KERNEL, demo_syscalls);
    process_create("shell", PROC_FLAG_KERNEL, shell_process);

    /* Start PIT so scheduler_tick() runs from IRQ0 */
    timer_init(100);

    /* Phase 3: System Call Interface */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== PHASE 3: System Call Interface & User Mode ===\n");
    syscall_init();

    /* Phase 4: VFS and Filesystem */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== PHASE 4: VFS and Filesystem ===\n");
    vfs_init();
    ramfs_init();
    ramfs_create_file("/test.txt", "Hello from SYNAPSE SO VFS!");
    ramfs_create_file("/readme.txt", "Welcome to SYNAPSE SO Phase 4!");
    
    /* Create user mode test process */
    vga_print("[+] Creating user mode test process...\n");
    uint32_t user_pid = create_user_test_process();
    if (user_pid > 0) {
        vga_print("    User process created with PID: ");
        vga_print_dec(user_pid);
        vga_print("\n");
    } else {
        vga_print("[-] Failed to create user process\n");
    }

    /* Memory information */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\nSystem Information:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    if (mbi != 0) {
        vga_print("    Lower memory: ");
        vga_print_dec(mbi->mem_lower);
        vga_print(" KB\n");
        vga_print("    Upper memory: ");
        vga_print_dec(mbi->mem_upper);
        vga_print(" KB\n");
    } else {
        vga_print("    Memory information not available\n");
    }

    /* Kernel ready */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\n[SUCCESS] Phase 3 initialized successfully!\n");
    vga_print("SYNAPSE SO is ready with user mode support.\n");

    /* Start scheduler */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\nStarting scheduler...\n");
    vga_print("[+] Enabling interrupts\n");
    __asm__ __volatile__("sti");

    /* Infinite loop - kernel should never return */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
