/* SYNAPSE SO - Virtual Memory Manager Implementation */
/* Licensed under GPLv3 */

#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/vga.h>

/* Kernel page directory */
static page_directory_t* kernel_directory;

/* Current page directory */
static page_directory_t* current_directory;

/* Physical address of kernel page directory */
static uint32_t kernel_pd_phys;

/* Kernel virtual address space starts at 3GB */
#define KERNEL_VIRT_START 0xC0000000

/* Physical memory mapped at kernel start */
#define KERNEL_PHYS_BASE 0x100000

/* Get page table index from virtual address */
static inline uint32_t get_table_index(uint32_t virt_addr) {
    return (virt_addr >> 22) & 0x3FF;
}

/* Get page index from virtual address */
static inline uint32_t get_page_index(uint32_t virt_addr) {
    return (virt_addr >> 12) & 0x3FF;
}

/* Get page directory entry */
static inline uint32_t* get_pde(page_directory_t* pd, uint32_t virt_addr) {
    return &pd->entries[get_table_index(virt_addr)];
}

/* Get page table entry */
static inline uint32_t* get_pte(page_directory_t* pd, uint32_t virt_addr) {
    uint32_t* pde = get_pde(pd, virt_addr);
    if (!(*pde & PAGE_PRESENT)) {
        return 0;
    }
    /* Convert physical page table address to kernel virtual address before dereferencing */
    page_table_t* pt = (page_table_t*)(((*pde) & 0xFFFFF000) + KERNEL_VIRT_START);
    return &pt->entries[get_page_index(virt_addr)];
}

/* Initialize virtual memory manager */
void vmm_init(void) {
    vga_print("[+] Initializing Virtual Memory Manager...\n");

    /* Allocate kernel page directory */
    kernel_pd_phys = pmm_alloc_frame();
    if (kernel_pd_phys == 0) {
        vga_print("[-] Failed to allocate kernel page directory!\n");
        return;
    }
    kernel_directory = (page_directory_t*)(kernel_pd_phys + KERNEL_VIRT_START);
    current_directory = kernel_directory;

    /* Clear page directory */
    for (uint32_t i = 0; i < 1024; i++) {
        kernel_directory->entries[i] = 0;
    }

    /* Map kernel space (identity mapping for first 4MB) */
    for (uint32_t i = 0; i < 0x400000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Map kernel to higher half (3GB+) */
    for (uint32_t i = 0x100000; i < 0x200000; i += PAGE_SIZE) {
        vmm_map_page(i + KERNEL_VIRT_START - KERNEL_PHYS_BASE, i,
                     PAGE_PRESENT | PAGE_WRITE);
    }

    /* Map frames bitmap */
    for (uint32_t i = 0x200000; i < 0x300000; i += PAGE_SIZE) {
        vmm_map_page(i + KERNEL_VIRT_START - KERNEL_PHYS_BASE, i,
                     PAGE_PRESENT | PAGE_WRITE);
    }

    /* Enable paging - use the saved physical address directly */
    __asm__ volatile(
        "mov %0, %%cr3\n"     /* Load CR3 with page directory physical address */
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"  /* Set paging bit */
        "mov %%eax, %%cr0\n"
        :
        : "r"(kernel_pd_phys)
        : "%eax"
    );

    vga_print("    Paging enabled\n");
}

/* Map a virtual page to a physical page */
void vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t table_idx = get_table_index(virt_addr);
    uint32_t page_idx = get_page_index(virt_addr);

    /* Check if page table exists */
    uint32_t* pde = &current_directory->entries[table_idx];
    page_table_t* pt;

    if (!(*pde & PAGE_PRESENT)) {
        /* Allocate new page table */
        uint32_t pt_phys = pmm_alloc_frame();
        if (pt_phys == 0) {
            vga_print("[-] Failed to allocate page table!\n");
            /* Allocation failure during page table creation is fatal during boot: halt to avoid enabling paging with incomplete mappings. */
            __asm__ volatile("cli; hlt");
        }
        pt = (page_table_t*)(pt_phys + KERNEL_VIRT_START);

        /* Clear page table */
        for (uint32_t i = 0; i < 1024; i++) {
            pt->entries[i] = 0;
        }

        /* Set page directory entry */
        *pde = pt_phys | flags | PAGE_PRESENT;
    } else {
        /* Convert PDE physical address to kernel virtual address */
        pt = (page_table_t*)(((*pde) & 0xFFFFF000) + KERNEL_VIRT_START);
    }

    /* Map the page */
    pt->entries[page_idx] = phys_addr | flags | PAGE_PRESENT;

    /* Flush TLB */
    vmm_flush_tlb(virt_addr);
}

/* Unmap a virtual page */
void vmm_unmap_page(uint32_t virt_addr) {
    uint32_t* pte = get_pte(current_directory, virt_addr);

    if (pte && (*pte & PAGE_PRESENT)) {
        /* Free the frame */
        pmm_free_frame(*pte & 0xFFFFF000);

        /* Clear the entry */
        *pte = 0;

        /* Flush TLB */
        vmm_flush_tlb(virt_addr);
    }
}

/* Unmap a virtual page without freeing the physical frame */
void vmm_unmap_page_no_free(uint32_t virt_addr) {
    uint32_t* pte = get_pte(current_directory, virt_addr);

    if (pte && (*pte & PAGE_PRESENT)) {
        /* Clear the entry but DON'T free the frame */
        *pte = 0;

        /* Flush TLB */
        vmm_flush_tlb(virt_addr);
    }
}

/* Get physical address of a virtual page */
uint32_t vmm_get_phys_addr(uint32_t virt_addr) {
    uint32_t* pte = get_pte(current_directory, virt_addr);

    if (!pte || !(*pte & PAGE_PRESENT)) {
        return 0;
    }

    return (*pte & 0xFFFFF000) + (virt_addr & 0xFFF);
}

/* Allocate a new page directory for a process */
page_directory_t* vmm_create_page_directory(void) {
    /* Allocate page directory */
    uint32_t pd_phys = pmm_alloc_frame();
    if (pd_phys == 0) {
        vga_print("[-] Failed to allocate page directory!\n");
        return 0;
    }
    page_directory_t* pd = (page_directory_t*)(pd_phys + KERNEL_VIRT_START);

    /* Clear page directory */
    for (uint32_t i = 0; i < 1024; i++) {
        pd->entries[i] = 0;
    }

    /* Copy kernel mappings (last 256 entries, starting at 768) */
    for (uint32_t i = 768; i < 1024; i++) {
        pd->entries[i] = kernel_directory->entries[i];
    }

    return pd;
}

/* Switch to a new page directory */
void vmm_switch_page_directory(page_directory_t* pd) {
    if (pd == 0) {
        vga_print("[-] Cannot switch to null page directory!\n");
        return;
    }

    current_directory = pd;

    /* Calculate physical address from virtual address */
    uint32_t pd_phys = (uint32_t)pd - KERNEL_VIRT_START;

    /* Load CR3 with physical address */
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys));
}

/* Page fault handler */
void vmm_page_fault_handler(uint32_t error_code) {
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    vga_print("\n[-] PAGE FAULT!\n");
    vga_print("    Fault address: 0x");
    vga_print_hex(fault_addr);
    vga_print("\n");
    vga_print("    Error code: 0x");
    vga_print_hex(error_code);
    vga_print("\n");

    if (error_code & PF_PRESENT) {
        vga_print("    Page was present\n");
        
        /* Check if this is a COW page fault */
        if (error_code & PF_WRITE && vmm_is_page_cow(fault_addr)) {
            vga_print("    COW page fault detected\n");
            
            /* Handle COW page fault */
            if (vmm_handle_cow_fault(fault_addr) == 0) {
                vga_print("    COW page fault handled successfully\n");
                return;  /* Fault handled, continue execution */
            } else {
                vga_print("    Failed to handle COW page fault\n");
            }
        }
    } else {
        vga_print("    Page not present\n");
    }

    if (error_code & PF_WRITE) {
        vga_print("    Write operation\n");
    } else {
        vga_print("    Read operation\n");
    }

    if (error_code & PF_USER) {
        vga_print("    User mode\n");
    } else {
        vga_print("    Kernel mode\n");
    }

    if (error_code & PF_RESERVED) {
        vga_print("    Reserved bit set\n");
    }

    if (error_code & PF_INSTRUCTION) {
        vga_print("    Instruction fetch\n");
    }

    /* Halt the system */
    __asm__ volatile("hlt");
}

/* Get current page directory */
page_directory_t* vmm_get_current_directory(void) {
    return current_directory;
}

/* Get current CR3 value (physical address of page directory) */
uint32_t vmm_get_cr3(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

/* Temporary mapping slot bitmap */
static uint32_t temp_slots_bitmap[(TEMP_MAPPING_PAGES + 31) / 32];

/* Allocate a temporary mapping slot */
int vmm_alloc_temp_slot(void) {
    /* Find a free slot */
    for (uint32_t i = 0; i < TEMP_MAPPING_PAGES; i++) {
        uint32_t bitmap_idx = i / 32;
        uint32_t bit_idx = i % 32;
        
        if (!(temp_slots_bitmap[bitmap_idx] & (1 << bit_idx))) {
            /* Mark slot as used */
            temp_slots_bitmap[bitmap_idx] |= (1 << bit_idx);
            return (int)i;
        }
    }
    
    /* No free slots */
    return -1;
}

/* Free a temporary mapping slot */
void vmm_free_temp_slot(int slot) {
    if (slot < 0 || slot >= (int)TEMP_MAPPING_PAGES) {
        return;
    }
    
    uint32_t bitmap_idx = slot / 32;
    uint32_t bit_idx = slot % 32;
    
    /* Mark slot as free */
    temp_slots_bitmap[bitmap_idx] &= ~(1 << bit_idx);
}

/* Map a physical frame to a temporary virtual address using a specific slot */
uint32_t vmm_map_temp_page(uint32_t phys_addr, int slot) {
    if (slot < 0 || slot >= (int)TEMP_MAPPING_PAGES) {
        return 0;
    }
    
    /* Calculate virtual address */
    uint32_t virt_addr = TEMP_MAPPING_BASE + (slot * PAGE_SIZE);
    
    /* Map the page with kernel-only permissions in the currently active directory
     * This works because:
     * 1. Syscalls execute in kernel mode even with process CR3
     * 2. The mapping is kernel-only (no PAGE_USER), so user mode can't access it
     * 3. The mapping only needs to exist during this syscall
     */
    vmm_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
    
    return virt_addr;
}

/* Unmap a temporary page for a specific slot */
void vmm_unmap_temp_page(int slot) {
    if (slot < 0 || slot >= (int)TEMP_MAPPING_PAGES) {
        return;
    }
    
    /* Calculate virtual address */
    uint32_t virt_addr = TEMP_MAPPING_BASE + (slot * PAGE_SIZE);
    
    /* Unmap without freeing the physical frame from the currently active directory */
    vmm_unmap_page_no_free(virt_addr);
}
