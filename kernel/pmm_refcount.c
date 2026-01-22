/* SYNAPSE SO - PMM Reference Counting (Stub Implementation) */
/* Licensed under GPLv3 */

#include <kernel/pmm.h>
#include <kernel/vga.h>

/* Reference count table - stub for now */
static uint16_t* frame_refcounts = 0;
static uint32_t num_frames_total = 0;

/* Initialize reference counting (called from pmm_init) */
void pmm_refcount_init(uint32_t total_frames) {
    num_frames_total = total_frames;
    
    /* TODO: Allocate refcount table
     * frame_refcounts = (uint16_t*)pmm_kmalloc(total_frames * sizeof(uint16_t));
     * Initialize all to 0
     */
    
    vga_print("    Reference counting: STUB (not yet implemented)\n");
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
    
    /* TODO: Implement
     * if (frame_refcounts[frame_num] < 0xFFFF) {
     *     frame_refcounts[frame_num]++;
     * }
     */
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
    
    /* TODO: Implement
     * if (frame_refcounts[frame_num] > 0) {
     *     frame_refcounts[frame_num]--;
     *     if (frame_refcounts[frame_num] == 0) {
     *         pmm_free_frame(frame_addr);
     *     }
     * }
     */
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
    
    /* TODO: Implement
     * return frame_refcounts[frame_num];
     */
    return 0;
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
    stats->shared_frames = 0;  /* TODO: Count frames with refcount > 1 */
}
