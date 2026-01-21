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

/* Map a physical frame to a temporary virtual address */
uint32_t vmm_map_temp_page(uint32_t phys_addr);

/* Unmap a temporary page */
void vmm_unmap_temp_page(uint32_t virt_addr);

/* Get current CR3 (physical address of page directory) */
uint32_t vmm_get_cr3(void);

#endif /* KERNEL_VMM_H */
