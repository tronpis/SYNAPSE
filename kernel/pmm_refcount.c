/* SYNAPSE SO - PMM Reference Counting Implementation */
/* Licensed under GPLv3 */

#include <kernel/pmm.h>
#include <kernel/vga.h>

/* Reference count table */
static uint16_t* frame_refcounts = 0;
static uint32_t num_frames_total = 0;

/* Initialize reference counting (called from pmm_init) */
void pmm_refcount_init(uint32_t total_frames) {
    num_frames_total = total_frames;

    /* Allocate refcount table using early PMM heap.
       This runs before the full kernel heap (kmalloc) is initialized. */
    frame_refcounts = (uint16_t*)pmm_kmalloc(total_frames * sizeof(uint16_t));
    if (frame_refcounts == 0) {
        vga_print("[-] Failed to allocate reference count table\n");
        return;
    }
    
    /* Initialize all reference counts to 0 */
    for (uint32_t i = 0; i < total_frames; i++) {
        frame_refcounts[i] = 0;
    }
    
    vga_print("    Reference counting: Initialized for ");
    vga_print_dec(total_frames);
    vga_print(" frames\n");
}

/* Increment reference count */
void pmm_ref_frame(uint32_t frame_addr) {
    if (frame_refcounts == 0) {
        return;  /* Not initialized */
    }
    
    uint32_t frame_num = frame_addr / FRAME_SIZE;
    if (frame_num >= num_frames_total) {
        return;
    }
    
    /* Increment reference count if not at maximum */
    if (frame_refcounts[frame_num] < 0xFFFF) {
        frame_refcounts[frame_num]++;
    }
}

/* Decrement reference count, free if reaches 0 */
void pmm_unref_frame(uint32_t frame_addr) {
    if (frame_refcounts == 0) {
        return;  /* Not initialized */
    }

    uint32_t frame_num = frame_addr / FRAME_SIZE;
    if (frame_num >= num_frames_total) {
        return;
    }

    /* Decrement reference count if greater than 0 */
    if (frame_refcounts[frame_num] > 0) {
        frame_refcounts[frame_num]--;
    }
}

/* Get reference count */
uint32_t pmm_get_ref_count(uint32_t frame_addr) {
    if (frame_refcounts == 0) {
        return 0;  /* Not initialized */
    }
    
    uint32_t frame_num = frame_addr / FRAME_SIZE;
    if (frame_num >= num_frames_total) {
        return 0;
    }
    
    return frame_refcounts[frame_num];
}

/* Get PMM statistics */
void pmm_get_stats(pmm_stats_t* stats) {
    if (stats == 0) {
        return;
    }
    
    /* Fill with basic stats */
    stats->total_frames = pmm_get_free_frames() + pmm_get_used_frames();
    stats->used_frames = pmm_get_used_frames();
    stats->free_frames = pmm_get_free_frames();
    
    /* Count shared frames (frames with refcount > 1) */
    stats->shared_frames = 0;
    if (frame_refcounts != 0) {
        for (uint32_t i = 0; i < num_frames_total; i++) {
            if (frame_refcounts[i] > 1) {
                stats->shared_frames++;
            }
        }
    }
}
