/* SYNAPSE SO - Exec System Call Implementation */
/* Licensed under GPLv3 */

#include <kernel/exec.h>
#include <kernel/process.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <kernel/syscall.h>

/* Exec system call implementation */
int do_exec(const char* path, char* const argv[]) {
    (void)argv;  /* Not implemented yet */

    process_t* current = process_get_current();
    if (current == 0) {
        return -1;
    }

    vga_print("[+] exec() called: ");
    if (path != 0) {
        vga_print(path);
    } else {
        vga_print("(null)");
    }
    vga_print("\n");

    /* For now, assume path is actually a pointer to ELF data in memory */
    /* In a real system, we would read from filesystem */
    if (path == 0) {
        vga_print("[-] exec: No program path specified\n");
        return -1;
    }

    /* Validate pointer is in user space */
    if ((uint32_t)path >= 0xC0000000) {
        vga_print("[-] exec: Path in kernel space\n");
        return -1;
    }

    /* Check if it's an ELF binary */
    elf32_header_t* header = (elf32_header_t*)path;
    if (elf_check_header(header) != 0) {
        vga_print("[-] exec: Not a valid ELF binary\n");
        return -1;
    }

    /* For kernel processes, load directly */
    if (current->flags & PROC_FLAG_KERNEL) {
        vga_print("[-] exec: Cannot exec kernel processes\n");
        return -1;
    }

    /* Save old page directory */
    page_directory_t* old_dir = current->page_dir;

    /* Create new page directory for the new program */
    page_directory_t* new_dir = vmm_create_page_directory();
    if (new_dir == 0) {
        vga_print("[-] exec: Failed to create new page directory\n");
        return -1;
    }

    /* Switch to new page directory */
    vmm_switch_page_directory(new_dir);

    /* Load ELF binary into process */
    uint32_t elf_size = 4096;  /* Assume maximum of 4KB for test */
    if (elf_load_to_process((uint8_t*)path, elf_size, current) != 0) {
        vga_print("[-] exec: Failed to load ELF binary\n");
        vmm_switch_page_directory(old_dir);
        vmm_destroy_page_directory(new_dir);
        return -1;
    }

    /* Allocate user stack */
    uint32_t stack_phys = pmm_alloc_frame();
    if (stack_phys == 0) {
        vga_print("[-] exec: Failed to allocate stack\n");
        vmm_switch_page_directory(old_dir);
        vmm_destroy_page_directory(new_dir);
        return -1;
    }

    uint32_t stack_virt = 0x7FFFF000;
    vmm_map_page(stack_virt, stack_phys,
                 PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    current->stack_start = stack_virt - USER_STACK_SIZE;
    current->stack_end = stack_virt;

    /* Get entry point from ELF header */
    uint32_t entry_point = header->e_entry;
    vga_print("[+] exec: Entry point at 0x");
    vga_print_hex(entry_point);
    vga_print("\n");

    /* Set up process to start at entry point */
    current->eip = entry_point;
    current->esp = current->stack_end;
    current->ebp = current->stack_end;
    current->eflags = 0x202;  /* Interrupts enabled */

    /* Clear registers */
    current->eax = 0;
    current->ebx = 0;
    current->ecx = 0;
    current->edx = 0;
    current->esi = 0;
    current->edi = 0;

    /* Update page directory */
    current->page_dir = new_dir;

    /* Destroy old page directory (in a real system, we'd free the pages too) */
    /* For now, just switch back and clean up later */
    vmm_switch_page_directory(old_dir);

    /* Free the old directory structure */
    /* Note: In a real system, we'd need to free all the physical pages */

    vga_print("[+] exec: Successfully loaded program\n");
    return 0;
}

