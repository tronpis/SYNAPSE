/* SYNAPSE SO - Physical Memory Manager Implementation */
/* Licensed under GPLv3 */

#include <kernel/pmm.h>
#include <kernel/vga.h>
#include <kernel/io.h>

/* Bitmap for tracking frames */
/* Each bit represents one 4KB frame */
static uint32_t* frames_bitmap;
static uint32_t total_frames;
static uint32_t used_frames;
static uint32_t last_used_frame;

/* Physical memory information */
static uint32_t total_memory;

/* Kernel heap for pre-paging allocations */
static uint8_t* kernel_heap;
static uint32_t kernel_heap_size;
static uint32_t kernel_heap_used;

/* Memory types (from Multiboot spec) */
#define MEM_TYPE_AVAILABLE 1
#define MEM_TYPE_RESERVED 2
#define MEM_TYPE_ACPI_RECLAIMABLE 3
#define MEM_TYPE_NVS 4
#define MEM_TYPE_UNUSABLE 5

/* Get frame index from address */
static inline uint32_t addr_to_frame(uint32_t addr) {
    return addr / FRAME_SIZE;
}

/* Get address from frame index */
static inline uint32_t frame_to_addr(uint32_t frame) {
    return frame * FRAME_SIZE;
}

/* Test if a frame is free */
static inline int frame_is_free(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    return !(frames_bitmap[index] & (1 << bit));
}

/* Set frame as used */
static inline void frame_set_used(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    frames_bitmap[index] |= (1 << bit);
    used_frames++;
}

/* Set frame as free */
static inline void frame_set_free(uint32_t frame) {
    uint32_t index = frame / 32;
    uint32_t bit = frame % 32;
    frames_bitmap[index] &= ~(1 << bit);
    used_frames--;
}

/* Initialize PMM */
void pmm_init(mem_map_t* mmap, uint32_t mmap_size, uint32_t mmap_desc_size) {
    vga_print("[+] Initializing Physical Memory Manager...\n");

    /* Calculate total memory from memory map */
    mem_map_entry_t* entry = mmap->entries;
    total_memory = 0;
    uint32_t num_entries = mmap_size / mmap_desc_size;

    /* Find highest memory address */
    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t end = entry->base_addr_low + entry->length_low;
        if (entry->type == MEM_TYPE_AVAILABLE && end > total_memory) {
            total_memory = end;
        }
        entry = (mem_map_entry_t*)((uint32_t)entry + mmap_desc_size);
    }

    /* Calculate total frames */
    total_frames = total_memory / FRAME_SIZE;
    used_frames = 0;
    last_used_frame = 0;

    /* Calculate bitmap size (in bytes) */
    uint32_t bitmap_size = (total_frames + 31) / 32 * 4;

    /* Place bitmap after kernel (assume kernel ends at 2MB for now) */
    frames_bitmap = (uint32_t*)0x200000;

    /* Clear bitmap */
    for (uint32_t i = 0; i < bitmap_size / 4; i++) {
        frames_bitmap[i] = 0;
    }

    /* Mark all frames as used initially, then mark available ones as free */
    for (uint32_t i = 0; i < total_frames; i++) {
        frame_set_used(i);
    }

    /* Mark available frames as free */
    entry = mmap->entries;
    for (uint32_t i = 0; i < num_entries; i++) {
        if (entry->type == MEM_TYPE_AVAILABLE) {
            uint32_t start_frame = addr_to_frame(entry->base_addr_low);
            uint32_t end_frame = addr_to_frame(entry->base_addr_low + entry->length_low);

            for (uint32_t f = start_frame; f < end_frame; f++) {
                if (frame_is_free(f)) {
                    continue;
                }
                frame_set_free(f);
            }
        }
        entry = (mem_map_entry_t*)((uint32_t)entry + mmap_desc_size);
    }

    /* Mark kernel region as used (1MB to 2MB for now) */
    uint32_t kernel_start_frame = addr_to_frame(0x100000);
    uint32_t kernel_end_frame = addr_to_frame(0x200000);
    for (uint32_t f = kernel_start_frame; f < kernel_end_frame; f++) {
        if (!frame_is_free(f)) {
            frame_set_used(f);
        }
    }

    /* Mark bitmap area as used */
    uint32_t bitmap_start_frame = addr_to_frame((uint32_t)frames_bitmap);
    uint32_t bitmap_end_frame = addr_to_frame((uint32_t)frames_bitmap + bitmap_size);
    for (uint32_t f = bitmap_start_frame; f < bitmap_end_frame; f++) {
        if (!frame_is_free(f)) {
            frame_set_used(f);
        }
    }

    /* Initialize reference counting */
    pmm_refcount_init(total_frames);

    vga_print("    Total memory: ");
    vga_print_dec(total_memory / 1024 / 1024);
    vga_print(" MB\n");
    vga_print("    Total frames: ");
    vga_print_dec(total_frames);
    vga_print("\n");
    vga_print("    Free frames: ");
    vga_print_dec(pmm_get_free_frames());
    vga_print("\n");
}

/* Allocate a physical frame */
uint32_t pmm_alloc_frame(void) {
    /* Start from last used frame for better locality */
    uint32_t start_frame = last_used_frame;

    for (uint32_t i = 0; i < total_frames; i++) {
        uint32_t frame = (start_frame + i) % total_frames;

        if (frame_is_free(frame)) {
            frame_set_used(frame);
            last_used_frame = frame;
            
            /* Initialize reference count to 1 for newly allocated frames */
            pmm_ref_frame(frame_to_addr(frame));
            
            return frame_to_addr(frame);
        }
    }

    /* No free frames available */
    vga_print("[-] Error: Out of physical memory!\n");
    return 0;
}

/* Free a physical frame */
void pmm_free_frame(uint32_t frame_addr) {
    uint32_t frame = addr_to_frame(frame_addr);

    if (frame >= total_frames) {
        return;
    }

    if (frame_is_free(frame)) {
        return;
    }

    /* Use reference counting to manage frame lifecycle */
    pmm_unref_frame(frame_addr);
    
    /* Only mark as free if reference count reaches 0 */
    if (pmm_get_ref_count(frame_addr) == 0) {
        frame_set_free(frame);
    }
}

/* Get number of free frames */
uint32_t pmm_get_free_frames(void) {
    return total_frames - used_frames;
}

/* Get number of used frames */
uint32_t pmm_get_used_frames(void) {
    return used_frames;
}

/* Initialize simple kernel heap for pre-paging allocations */
void pmm_init_kernel_heap(uint32_t start, uint32_t size) {
    kernel_heap = (uint8_t*)start;
    kernel_heap_size = size;
    kernel_heap_used = 0;
}

/* Simple kernel malloc (before paging) */
void* pmm_kmalloc(uint32_t size) {
    if (kernel_heap_used + size > kernel_heap_size) {
        return 0;
    }

    void* ptr = kernel_heap + kernel_heap_used;
    kernel_heap_used += size;
    return ptr;
}

/* Simple kernel free (no-op for simple implementation) */
void pmm_kfree(void* ptr, uint32_t size) {
    /* No-op for simple implementation */
    (void)ptr;
    (void)size;
}
