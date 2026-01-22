/* SYNAPSE SO - Virtual Memory Manager */
/* Licensed under GPLv3 */

#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include <stdint.h>

/* Page size is 4KB */
#define PAGE_SIZE 4096

/* Page table entry flags */
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_NOCACHE    (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_GLOBAL     (1 << 8)
#define PAGE_COW        (1 << 9)  /* Copy-on-Write flag (custom, uses available bit) */
#define PAGE_FRAME(addr) ((addr) & 0xFFFFF000)

/* Page directory and table structures */
typedef struct {
    uint32_t entries[1024];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

typedef struct {
    uint32_t entries[1024];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

/* Page fault error codes */
#define PF_PRESENT  (1 << 0)
#define PF_WRITE    (1 << 1)
#define PF_USER     (1 << 2)
#define PF_RESERVED (1 << 3)
#define PF_INSTRUCTION (1 << 4)

/* Initialize virtual memory manager */
void vmm_init(void);

/* Map a virtual page to a physical page */
void vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/* Unmap a virtual page */
void vmm_unmap_page(uint32_t virt_addr);

/* Unmap a virtual page without freeing the physical frame */
void vmm_unmap_page_no_free(uint32_t virt_addr);

/* Get physical address of a virtual page */
uint32_t vmm_get_phys_addr(uint32_t virt_addr);

/* Allocate a new page directory for a process */
page_directory_t* vmm_create_page_directory(void);

/* Switch to a new page directory */
void vmm_switch_page_directory(page_directory_t* pd);

/* Page fault handler */
void vmm_page_fault_handler(uint32_t error_code);

/* Flush TLB entry */
static inline void vmm_flush_tlb(uint32_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

/* Get current page directory */
page_directory_t* vmm_get_current_directory(void);

/* Temporary mapping area for Phase 3: copy data between address spaces */
#define TEMP_MAPPING_BASE 0xE0000000  /* Temporary mapping region at 3.5GB */
#define TEMP_MAPPING_PAGES 256          /* 256 pages = 1MB */

/* Allocate a temporary mapping slot (returns slot index or -1 on failure) */
int vmm_alloc_temp_slot(void);

/* Free a temporary mapping slot */
void vmm_free_temp_slot(int slot);

/* Map a physical frame to a temporary virtual address using a specific slot */
uint32_t vmm_map_temp_page(uint32_t phys_addr, int slot);

/* Unmap a temporary page for a specific slot */
void vmm_unmap_temp_page(int slot);

/* Get current CR3 (physical address of page directory) */
uint32_t vmm_get_cr3(void);

/* Copy-on-Write (COW) support */
/* Clone a page directory for fork() - marks pages as COW */
page_directory_t* vmm_clone_page_directory(page_directory_t* src);

/* Handle COW page fault - copies page on write */
int vmm_handle_cow_fault(uint32_t fault_addr);

/* Check if a page is marked as COW */
int vmm_is_page_cow(uint32_t virt_addr);

/* Memory statistics */
typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
    uint32_t cow_pages;
    uint32_t shared_pages;
} vmm_stats_t;

/* Get VMM statistics */
void vmm_get_stats(vmm_stats_t* stats);

#endif /* KERNEL_VMM_H */
