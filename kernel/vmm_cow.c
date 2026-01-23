/* SYNAPSE SO - VMM Copy-on-Write Implementation */
/* Licensed under GPLv3 */

#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/vga.h>
#include <kernel/string.h>

/* Clone page directory for fork() */
page_directory_t* vmm_clone_page_directory(page_directory_t* src) {
    if (src == 0) {
        return 0;
    }
    
    /* Create new page directory */
    page_directory_t* new_dir = vmm_create_page_directory();
    if (new_dir == 0) {
        vga_print("[-] Failed to create new page directory for clone\n");
        return 0;
    }
    
    /* Clone user space pages (first 768 entries) */
    for (uint32_t i = 0; i < 768; i++) {
        uint32_t src_pde = src->entries[i];
        
        if (src_pde & PAGE_PRESENT) {
            /* Get source page table */
            page_table_t* src_pt = (page_table_t*)((src_pde & 0xFFFFF000) + KERNEL_VIRT_START);
            
            /* Create new page table for this directory entry */
            uint32_t new_pt_phys = pmm_alloc_frame();
            if (new_pt_phys == 0) {
                vga_print("[-] Failed to allocate page table for clone\n");
                vmm_switch_page_directory(vmm_get_current_directory());
                return 0;
            }
            
            page_table_t* new_pt = (page_table_t*)(new_pt_phys + KERNEL_VIRT_START);
            
            /* Clear new page table */
            memset(new_pt, 0, PAGE_SIZE);
            
            /* Copy page table entries and mark as COW */
            for (uint32_t j = 0; j < 1024; j++) {
                uint32_t src_pte = src_pt->entries[j];
                
                if (src_pte & PAGE_PRESENT) {
                    /* Copy PTE but mark as read-only and COW */
                    uint32_t new_pte = (src_pte & ~PAGE_WRITE) | PAGE_COW | PAGE_PRESENT;
                    new_pt->entries[j] = new_pte;
                    
                    /* Increment reference count for the physical frame */
                    uint32_t frame_addr = src_pte & 0xFFFFF000;
                    pmm_ref_frame(frame_addr);
                }
            }
            
            /* Set new page directory entry */
            new_dir->entries[i] = new_pt_phys | (src_pde & ~PAGE_WRITE) | PAGE_COW | PAGE_PRESENT;
        }
    }
    
    vga_print("[+] Page directory cloned successfully\n");
    return new_dir;
}

/* Handle COW page fault */
int vmm_handle_cow_fault(uint32_t fault_addr) {
    uint32_t* pte = get_pte(current_directory, fault_addr);
    
    if (pte == 0 || !(*pte & PAGE_PRESENT) || !(*pte & PAGE_COW)) {
        return -1;  /* Not a COW page */
    }
    
    /* Get physical address of the original page */
    uint32_t original_phys = *pte & 0xFFFFF000;
    
    /* Allocate new frame for the copy */
    uint32_t new_phys = pmm_alloc_frame();
    if (new_phys == 0) {
        vga_print("[-] Failed to allocate frame for COW\n");
        return -1;
    }
    
    /* Copy data from original page to new page */
    uint8_t* src = (uint8_t*)(original_phys + KERNEL_VIRT_START);
    uint8_t* dest = (uint8_t*)(new_phys + KERNEL_VIRT_START);
    
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        dest[i] = src[i];
    }
    
    /* Update PTE to point to new frame with write permissions */
    *pte = new_phys | (PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    
    /* Decrement reference count for original frame */
    pmm_unref_frame(original_phys);
    
    /* Flush TLB */
    vmm_flush_tlb(fault_addr);
    
    vga_print("[+] COW page fault handled for address 0x");
    vga_print_hex(fault_addr);
    vga_print("\n");
    
    return 0;  /* Handled successfully */
}

/* Check if page is COW */
int vmm_is_page_cow(uint32_t virt_addr) {
    uint32_t* pte = get_pte(current_directory, virt_addr);
    
    if (pte == 0 || !(*pte & PAGE_PRESENT)) {
        return 0;
    }
    
    return (*pte & PAGE_COW) ? 1 : 0;
}

/* Get VMM statistics */
void vmm_get_stats(vmm_stats_t* stats) {
    if (stats == 0) {
        return;
    }
    
    /* Initialize counters */
    stats->total_pages = 0;
    stats->used_pages = 0;
    stats->free_pages = 0;
    stats->cow_pages = 0;
    stats->shared_pages = 0;
    
    /* Count pages in current directory */
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t pde = current_directory->entries[i];
        
        if (pde & PAGE_PRESENT) {
            page_table_t* pt = (page_table_t*)((pde & 0xFFFFF000) + KERNEL_VIRT_START);
            
            for (uint32_t j = 0; j < 1024; j++) {
                uint32_t pte = pt->entries[j];
                
                if (pte & PAGE_PRESENT) {
                    stats->total_pages++;
                    stats->used_pages++;
                    
                    if (pte & PAGE_COW) {
                        stats->cow_pages++;
                    }
                    
                    /* Check if frame is shared (refcount > 1) */
                    uint32_t frame_addr = pte & 0xFFFFF000;
                    if (pmm_get_ref_count(frame_addr) > 1) {
                        stats->shared_pages++;
                    }
                }
            }
        }
    }
    
    /* Calculate free pages */
    stats->free_pages = (1024 * 1024) - stats->used_pages;
}
