/* SYNAPSE SO - Early Boot Diagnostics Implementation */
/* Licensed under GPLv3 */

#include <stddef.h>

#include <kernel/early.h>
#include <kernel/vga.h>
#include <kernel/cpu.h>

/* Minimum system requirements */
#define MIN_MEMORY_KB 4096  /* 4MB minimum */
#define REQ_CPU_FEATURES (CPU_FEATURE_PSE | CPU_FEATURE_PAE)

/* Boot check statistics */
static int checks_passed = 0;
static int checks_warned = 0;
static int checks_failed = 0;

/* External multiboot info (defined in kernel.c) */
extern void* multiboot_info_ptr;
extern uint32_t multiboot_magic;

/* Boot checks array */
static boot_check_t boot_checks[] = {
    { "CPU Requirements", early_check_cpu, 1 },
    { "Memory Requirements", early_check_memory, 1 },
    { "Multiboot Info", early_check_multiboot, 0 },
    { "Kernel Integrity", early_check_kernel, 0 },
    { NULL, NULL, 0 }
};

/* Initialize early diagnostics */
void early_init(void) {
    checks_passed = 0;
    checks_warned = 0;
    checks_failed = 0;
}

/* Run all boot checks */
int early_run_checks(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== Boot Diagnostics ===\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    for (int i = 0; boot_checks[i].name != NULL; i++) {
        vga_print("[");
        vga_set_color(VGA_COLOR_LIGHT_YELLOW, VGA_COLOR_BLACK);
        vga_print("CHECK");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print("] ");
        vga_print(boot_checks[i].name);
        vga_print("...");
        
        int result = boot_checks[i].check_func();
        
        if (result == BOOT_CHECK_OK) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_print(" OK\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            checks_passed++;
        } else if (result == BOOT_CHECK_WARNING) {
            vga_set_color(VGA_COLOR_LIGHT_YELLOW, VGA_COLOR_BLACK);
            vga_print(" WARNING\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            checks_warned++;
        } else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print(" FAIL\n");
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            checks_failed++;
            
            if (boot_checks[i].is_fatal) {
                vga_print("\n");
                early_panic("Fatal boot check failed");
            }
        }
    }
    
    return checks_failed > 0 ? BOOT_CHECK_FATAL : 
           checks_warned > 0 ? BOOT_CHECK_WARNING : BOOT_CHECK_OK;
}

/* Check CPU requirements */
int early_check_cpu(void) {
    if (!cpu_has_cpuid()) {
        vga_print("\n  ERROR: CPUID not supported\n");
        return BOOT_CHECK_FATAL;
    }
    
    /* Check for required features */
    if (!cpu_has_feature(CPU_FEATURE_FPU)) {
        vga_print("\n  WARNING: No FPU\n");
        return BOOT_CHECK_WARNING;
    }
    
    /* PSE is nice to have but not required */
    if (!cpu_has_feature(CPU_FEATURE_PSE)) {
        vga_print("\n  INFO: No PSE (4MB pages)\n");
    }
    
    return BOOT_CHECK_OK;
}

/* Check minimum memory */
int early_check_memory(void) {
    /* This is a basic check - full memory detection in PMM */
    if (multiboot_info_ptr == 0) {
        return BOOT_CHECK_WARNING;  /* Can't verify, assume OK */
    }
    
    /* Multiboot info structure */
    typedef struct {
        uint32_t flags;
        uint32_t mem_lower;
        uint32_t mem_upper;
        /* ... */
    } __attribute__((packed)) mbi_t;
    
    mbi_t* mbi = (mbi_t*)multiboot_info_ptr;
    
    if ((mbi->flags & 0x01) == 0) {
        return BOOT_CHECK_WARNING;  /* No memory info */
    }
    
    uint32_t total_kb = mbi->mem_lower + mbi->mem_upper;
    
    if (total_kb < MIN_MEMORY_KB) {
        vga_print("\n  ERROR: Insufficient memory (");
        vga_print_dec(total_kb);
        vga_print(" KB < ");
        vga_print_dec(MIN_MEMORY_KB);
        vga_print(" KB)\n");
        return BOOT_CHECK_FATAL;
    }
    
    return BOOT_CHECK_OK;
}

/* Check multiboot info */
int early_check_multiboot(void) {
    if (multiboot_magic != 0x2BADB002) {
        vga_print("\n  ERROR: Invalid multiboot magic\n");
        return BOOT_CHECK_FATAL;
    }
    
    if (multiboot_info_ptr == 0) {
        vga_print("\n  WARNING: NULL multiboot info pointer\n");
        return BOOT_CHECK_WARNING;
    }
    
    return BOOT_CHECK_OK;
}

/* Basic kernel integrity check */
int early_check_kernel(void) {
    /* Check if kernel code is readable */
    extern void kernel_main(void);
    volatile uint8_t* kernel_start = (uint8_t*)&kernel_main;
    
    /* Try to read first byte */
    uint8_t byte = *kernel_start;
    (void)byte;  /* Suppress unused warning */
    
    /* If we got here, kernel is readable */
    return BOOT_CHECK_OK;
}

/* Print boot summary */
void early_print_summary(void) {
    vga_print("\n");
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("Boot Checks Summary:\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_print("  Passed: ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_dec(checks_passed);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("\n");
    
    if (checks_warned > 0) {
        vga_print("  Warnings: ");
        vga_set_color(VGA_COLOR_LIGHT_YELLOW, VGA_COLOR_BLACK);
        vga_print_dec(checks_warned);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print("\n");
    }
    
    if (checks_failed > 0) {
        vga_print("  Failed: ");
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_dec(checks_failed);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print("\n");
    }
}

/* Panic with error message */
void early_panic(const char* message) {
    __asm__ __volatile__("cli");
    
    vga_clear_screen();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    vga_print("\n\n");
    vga_print("  KERNEL PANIC - EARLY BOOT FAILURE  \n");
    vga_print("\n");
    
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print("Error: ");
    vga_print(message);
    vga_print("\n\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("The system cannot continue. Please check:\n");
    vga_print("  - CPU is i686 or newer\n");
    vga_print("  - At least 4MB of RAM\n");
    vga_print("  - Booted with GRUB or compatible bootloader\n");
    vga_print("\n");
    vga_print("System halted.\n");
    
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
