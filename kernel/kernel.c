/* SYNAPSE SO - Kernel Entry Point */
/* Licensed under GPLv3 */

#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

/* Multiboot information structure */
typedef struct {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    /* ... more fields not used in minimal version ... */
} __attribute__((packed)) multiboot_info_t;

/* Kernel main function - entry point from bootloader */
void kernel_main(unsigned int magic, multiboot_info_t* mbi) {
    /* Clear screen */
    vga_clear_screen();

    /* Print kernel banner */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("SYNAPSE SO - Open Source Operating System\n");
    vga_print("=========================================\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("Initializing Kernel...\n");

    /* Check multiboot magic */
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    if (magic != 0x2BADB002) {
        vga_print("[-] Error: Invalid Multiboot magic number!\n");
        while (1) {
            __asm__ __volatile__("hlt");
        }
    }
    vga_print("[+] Multiboot validated successfully\n");

    /* Check if mbi is valid */
    if (mbi == 0) {
        vga_print("[-] Warning: Multiboot info pointer is null\n");
    }

    /* Initialize GDT */
    vga_print("[+] Initializing Global Descriptor Table...\n");
    gdt_init();
    vga_print("    GDT loaded successfully\n");

    /* Initialize IDT */
    vga_print("[+] Initializing Interrupt Descriptor Table...\n");
    idt_init();
    vga_print("    IDT loaded successfully\n");

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
    vga_print("\n[SUCCESS] Kernel initialized successfully!\n");
    vga_print("SYNAPSE SO is ready.\n");

    /* Infinite loop - kernel should never return */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
