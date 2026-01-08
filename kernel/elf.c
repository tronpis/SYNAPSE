/* SYNAPSE SO - ELF Loader Implementation */
/* Licensed under GPLv3 */

#include <kernel/elf.h>
#include <kernel/process.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/heap.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <kernel/io.h>

/* Check ELF header */
int elf_check_header(elf32_header_t* header) {
    /* Check magic number */
    if (strncmp((char*)header->e_ident, ELFMAG, SELFMAG) != 0) {
        vga_print("[-] Invalid ELF magic\n");
        return -1;
    }

    /* Check class (32-bit) */
    if (header->e_ident[4] != ELFCLASS32) {
        vga_print("[-] Not a 32-bit ELF\n");
        return -1;
    }

    /* Check endianness (little endian) */
    if (header->e_ident[5] != ELFDATA2LSB) {
        vga_print("[-] Not little endian\n");
        return -1;
    }

    /* Check version */
    if (header->e_ident[6] != EV_CURRENT) {
        vga_print("[-] Invalid ELF version\n");
        return -1;
    }

    /* Check machine (x86) */
    if (header->e_machine != EM_386) {
        vga_print("[-] Not x86 ELF\n");
        return -1;
    }

    /* Check file type (executable) */
    if (header->e_type != ET_EXEC) {
        vga_print("[-] Not an executable ELF\n");
        return -1;
    }

    return 0;
}

/* Load ELF binary */
int elf_load(uint8_t* elf_data, uint32_t size, uint32_t* entry_point) {
    /* Validate ELF data size */
    if (size < sizeof(elf32_header_t)) {
        vga_print("[-] ELF data too small for header\n");
        return -1;
    }

    /* Get header */
    elf32_header_t* header = (elf32_header_t*)elf_data;

    /* Validate program header table fits in provided ELF data */
    if (header->e_phoff + (uint32_t)header->e_phnum * header->e_phentsize > size) {
        vga_print("[-] Program headers exceed ELF size\n");
        return -1;
    }

    vga_print("[+] Loading ELF binary...\n");

    /* Check header */
    if (elf_check_header(header) != 0) {
        return -1;
    }

    vga_print("    Entry point: 0x");
    vga_print_hex(header->e_entry);
    vga_print("\n");
    vga_print("    Program headers: ");
    vga_print_dec(header->e_phnum);
    vga_print("\n");

    /* Load program segments */
    elf32_phdr_t* phdr = (elf32_phdr_t*)(elf_data + header->e_phoff);

    for (uint32_t i = 0; i < header->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            vga_print("    Loading segment at 0x");
            vga_print_hex(phdr->p_vaddr);
            vga_print(" (size: ");
            vga_print_dec(phdr->p_memsz);
            vga_print(" bytes)\n");

            /* Validate sizes/offsets */
            if (phdr->p_filesz > phdr->p_memsz) {
                vga_print("[-] Segment file size larger than memory size\n");
                return -1;
            }
            if (phdr->p_offset + phdr->p_filesz > size) {
                vga_print("[-] Segment exceeds ELF data size\n");
                return -1;
            }

            /* Calculate number of pages needed */
            uint32_t start_page = phdr->p_vaddr & 0xFFFFF000;
            uint32_t end_page = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & 0xFFFFF000;

            /* Map pages */
            for (uint32_t addr = start_page; addr < end_page; addr += PAGE_SIZE) {
                uint32_t phys = pmm_alloc_frame();
                if (phys == 0) {
                    vga_print("[-] Failed to allocate physical frame\n");
                    return -1;
                }

                uint32_t flags = 0;
                if (phdr->p_flags & PF_W) {
                    flags |= PAGE_WRITE;
                }
                flags |= PAGE_PRESENT;

                vmm_map_page(addr, phys, flags);
            }

            /* Copy segment data */
            uint8_t* dest = (uint8_t*)phdr->p_vaddr;
            uint8_t* src = elf_data + phdr->p_offset;

            /* Copy file data */
            if (phdr->p_filesz > 0) {
                memcpy(dest, src, phdr->p_filesz);
            }

            /* Zero out bss */
            if (phdr->p_memsz > phdr->p_filesz) {
                memset(dest + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
            }
        }

        phdr++;
    }

    /* Set entry point */
    if (entry_point != 0) {
        *entry_point = header->e_entry;
    }

    vga_print("[+] ELF loaded successfully\n");
    return 0;
}

/* Load ELF binary into a process */
int elf_load_to_process(uint8_t* elf_data, uint32_t size, process_t* proc) {
    /* Validate ELF data size */
    if (size < sizeof(elf32_header_t)) {
        vga_print("[-] ELF data too small for header\n");
        return -1;
    }

    if (proc == 0) {
        vga_print("[-] Process is null\n");
        return -1;
    }

    /* Get header */
    elf32_header_t* header = (elf32_header_t*)elf_data;

    /* Validate program header table fits in provided ELF data */
    if (header->e_phoff + (uint32_t)header->e_phnum * header->e_phentsize > size) {
        vga_print("[-] Program headers exceed ELF size\n");
        return -1;
    }

    vga_print("[+] Loading ELF for process ");
    vga_print(proc->name);
    vga_print("...\n");

    /* Check header */
    if (elf_check_header(header) != 0) {
        return -1;
    }

    /* Save current page directory */
    page_directory_t* old_dir = vmm_get_current_directory();

    /* Load program segments */
    /* First pass: Map all pages in the process's address space */
    elf32_phdr_t* phdr = (elf32_phdr_t*)(elf_data + header->e_phoff);

    for (uint32_t i = 0; i < header->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            vga_print("    Mapping segment at 0x");
            vga_print_hex(phdr->p_vaddr);
            vga_print("\n");

            /* Validate sizes/offsets */
            if (phdr->p_filesz > phdr->p_memsz) {
                vga_print("[-] Segment file size larger than memory size\n");
                vga_print("[-] Restoring kernel directory\n");
                vmm_switch_page_directory(old_dir);
                return -1;
            }
            if (phdr->p_offset + phdr->p_filesz > size) {
                vga_print("[-] Segment exceeds ELF data size\n");
                vga_print("[-] Restoring kernel directory\n");
                vmm_switch_page_directory(old_dir);
                return -1;
            }

            /* Calculate number of pages needed */
            uint32_t start_page = phdr->p_vaddr & 0xFFFFF000;
            uint32_t end_page = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & 0xFFFFF000;

            /* Map pages to process directory */
            vmm_switch_page_directory(proc->page_dir);

            uint32_t alloc_failed = 0;
            for (uint32_t addr = start_page; addr < end_page; addr += PAGE_SIZE) {
                uint32_t phys = pmm_alloc_frame();
                if (phys == 0) {
                    vga_print("[-] Failed to allocate physical frame\n");
                    alloc_failed = 1;
                    break;
                }
                uint32_t flags = 0;

                if (phdr->p_flags & PF_W) {
                    flags |= PAGE_WRITE;
                }
                flags |= PAGE_PRESENT | PAGE_USER;

                vmm_map_page(addr, phys, flags);
            }

            if (alloc_failed) {
                vga_print("[-] Restoring kernel directory\n");
                vmm_switch_page_directory(old_dir);
                return -1;
            }
        }

        phdr++;
    }

    /* Second pass: Copy data from kernel space to process space */
    /* Switch to kernel directory to access ELF data */
    vmm_switch_page_directory(old_dir);

    phdr = (elf32_phdr_t*)(elf_data + header->e_phoff);

    for (uint32_t i = 0; i < header->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            vga_print("    Copying segment at 0x");
            vga_print_hex(phdr->p_vaddr);
            vga_print("\n");

            /* Copy segment data using temporary mapping */
            uint8_t* dest = (uint8_t*)phdr->p_vaddr;
            uint8_t* src = elf_data + phdr->p_offset;

            /* Switch to process directory for destination */
            vmm_switch_page_directory(proc->page_dir);

            /* Copy file data */
            if (phdr->p_filesz > 0) {
                /* This won't work properly - src is in kernel space, dest in process space */
                /* We need to map kernel pages into process space temporarily */
                /* For now, we'll skip the copy - this is a known limitation */
                /* TODO: Implement temporary mapping of ELF data into process space */
                vga_print("    [!] Skipping copy (requires temp mapping)\n");
            }

            /* Zero out bss */
            if (phdr->p_memsz > phdr->p_filesz) {
                memset(dest + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
            }

            /* Switch back to kernel directory for next iteration */
            vmm_switch_page_directory(old_dir);
        }

        phdr++;
    }

    /* Set process entry point */
    proc->eip = header->e_entry;

    vga_print("[+] ELF mapped for process (copy TODO)\n");

    /* Ensure we're back in kernel directory */
    vmm_switch_page_directory(old_dir);

    return 0;
}
