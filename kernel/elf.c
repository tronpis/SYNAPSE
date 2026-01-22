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
    /* Stay in kernel directory for access to ELF data */
    /* Use temporary mappings to copy to process space */

    phdr = (elf32_phdr_t*)(elf_data + header->e_phoff);

    for (uint32_t i = 0; i < header->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            vga_print("    Copying segment at 0x");
            vga_print_hex(phdr->p_vaddr);
            vga_print("\n");

            uint32_t src_offset = phdr->p_offset;
            uint32_t dest_addr = phdr->p_vaddr;
            uint32_t copy_size = phdr->p_filesz;

            /* Copy file data page by page using temporary mappings */
            while (copy_size > 0) {
                /* Calculate page-aligned addresses */
                uint32_t src_page = (uint32_t)(elf_data + src_offset) & 0xFFFFF000;
                uint32_t src_off = src_offset & 0xFFF;
                uint32_t dest_page = dest_addr & 0xFFFFF000;
                uint32_t dest_off = dest_addr & 0xFFF;

                /* Get physical address of source (kernel space) */
                uint32_t src_phys = vmm_get_phys_addr(src_page);
                if (src_phys == 0) {
                    vga_print("[-] Failed to get physical address of source\n");
                    vmm_switch_page_directory(old_dir);
                    return -1;
                }

                /* Get physical address of destination (process space) */
                vmm_switch_page_directory(proc->page_dir);
                uint32_t dest_phys = vmm_get_phys_addr(dest_page);
                vmm_switch_page_directory(old_dir);
                if (dest_phys == 0) {
                    vga_print("[-] Failed to get physical address of destination\n");
                    return -1;
                }

                /* Allocate temporary slot and map destination page */
                int slot = vmm_alloc_temp_slot();
                if (slot < 0) {
                    vga_print("[-] Failed to allocate temp slot\n");
                    vmm_switch_page_directory(old_dir);
                    return -1;
                }
                
                uint32_t temp_dest = vmm_map_temp_page(dest_phys, slot);
                if (temp_dest == 0) {
                    vga_print("[-] Failed to map temp page\n");
                    vmm_free_temp_slot(slot);
                    vmm_switch_page_directory(old_dir);
                    return -1;
                }

                /* Copy data */
                uint32_t bytes_to_copy = PAGE_SIZE - src_off;
                if (bytes_to_copy > PAGE_SIZE - dest_off) {
                    bytes_to_copy = PAGE_SIZE - dest_off;
                }
                if (bytes_to_copy > copy_size) {
                    bytes_to_copy = copy_size;
                }

                uint8_t* src_ptr = (uint8_t*)(src_page + src_off);
                uint8_t* dest_ptr = (uint8_t*)(temp_dest + dest_off);

                for (uint32_t j = 0; j < bytes_to_copy; j++) {
                    dest_ptr[j] = src_ptr[j];
                }

                /* Unmap temporary page and free slot */
                vmm_unmap_temp_page(slot);
                vmm_free_temp_slot(slot);

                /* Advance */
                src_offset += bytes_to_copy;
                dest_addr += bytes_to_copy;
                copy_size -= bytes_to_copy;
            }

            /* Zero out bss (in process space) */
            if (phdr->p_memsz > phdr->p_filesz) {
                uint32_t bss_start = phdr->p_vaddr + phdr->p_filesz;
                uint32_t bss_size = phdr->p_memsz - phdr->p_filesz;
                uint32_t bss_end = bss_start + bss_size;

                /* Zero BSS page by page */
                for (uint32_t addr = bss_start; addr < bss_end; addr += PAGE_SIZE) {
                    uint32_t page = addr & 0xFFFFF000;
                    uint32_t phys = vmm_get_phys_addr(page);
                    if (phys == 0) {
                        vga_print("[-] Failed to get BSS page physical address\n");
                        vmm_switch_page_directory(old_dir);
                        return -1;
                    }

                    int bss_slot = vmm_alloc_temp_slot();
                    if (bss_slot < 0) {
                        vga_print("[-] Failed to allocate temp slot for BSS\n");
                        vmm_switch_page_directory(old_dir);
                        return -1;
                    }
                    
                    uint32_t temp = vmm_map_temp_page(phys, bss_slot);
                    if (temp == 0) {
                        vga_print("[-] Failed to map temporary BSS page\n");
                        vmm_free_temp_slot(bss_slot);
                        vmm_switch_page_directory(old_dir);
                        return -1;
                    }
                    
                    uint32_t zero_start = (addr == bss_start) ? (addr & 0xFFF) : 0;
                    uint32_t zero_end = (addr + PAGE_SIZE > bss_end) ? (bss_end - addr) : PAGE_SIZE;

                    uint8_t* ptr = (uint8_t*)(temp + zero_start);
                    for (uint32_t j = zero_start; j < zero_end; j++) {
                        ptr[j] = 0;
                    }

                    vmm_unmap_temp_page(bss_slot);
                    vmm_free_temp_slot(bss_slot);
                }
            }
        }

        phdr++;
    }

    /* Set process entry point */
    proc->eip = header->e_entry;

    vga_print("[+] ELF loaded into process address space successfully\n");

    /* Ensure we're back in kernel directory */
    vmm_switch_page_directory(old_dir);

    return 0;
}
