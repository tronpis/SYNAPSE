/* SYNAPSE SO - Kernel Heap Manager Implementation */
/* Licensed under GPLv3 */

#include <kernel/heap.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/vga.h>
#include <kernel/string.h>

/* Heap start and size */
static void* heap_start;
static uint32_t heap_size;
static heap_block_t* heap_head;

/* Statistics */
static uint32_t heap_used;
static uint32_t heap_free;

/* Align size to alignment boundary */
static inline uint32_t align_size(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/* Find free block */
static heap_block_t* find_free_block(uint32_t size) {
    heap_block_t* block = heap_head;
    uint32_t needed = align_size(size, HEAP_ALIGN);

    while (block != 0) {
        if (block->is_free && block->size >= needed) {
            return block;
        }
        block = block->next;
    }

    return 0;
}

/* Split block if needed */
static void split_block(heap_block_t* block, uint32_t size) {
    uint32_t aligned_size = align_size(size, HEAP_ALIGN);
    uint32_t total_size = align_size(size + sizeof(heap_block_t), HEAP_ALIGN);

    if (block->size - total_size >= sizeof(heap_block_t) + HEAP_ALIGN) {
        /* Create new block */
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + total_size);
        new_block->size = block->size - total_size;
        new_block->magic = HEAP_MAGIC;
        new_block->is_free = 1;
        new_block->prev = block;
        new_block->next = block->next;

        if (block->next != 0) {
            block->next->prev = new_block;
        }

        block->next = new_block;
        block->size = aligned_size;

        heap_free += new_block->size;
    }
}

/* Merge adjacent free blocks */
static void merge_blocks(heap_block_t* block) {
    /* Merge with next block if free */
    if (block->next != 0 && block->next->is_free) {
        heap_block_t* next = block->next;

        block->size += next->size + sizeof(heap_block_t);
        block->next = next->next;

        if (next->next != 0) {
            next->next->prev = block;
        }

        heap_used -= sizeof(heap_block_t);
        heap_free += sizeof(heap_block_t);
    }

    /* Merge with previous block if free */
    if (block->prev != 0 && block->prev->is_free) {
        heap_block_t* prev = block->prev;

        prev->size += block->size + sizeof(heap_block_t);
        prev->next = block->next;

        if (block->next != 0) {
            block->next->prev = prev;
        }

        heap_used -= sizeof(heap_block_t);
        heap_free += sizeof(heap_block_t);
    }
}

/* Initialize kernel heap */
void heap_init(void* start, uint32_t size) {
    vga_print("[+] Initializing Kernel Heap...\n");

    heap_start = start;
    heap_size = size;
    heap_used = sizeof(heap_block_t);
    heap_free = size - sizeof(heap_block_t);

    /* Create initial block */
    heap_head = (heap_block_t*)start;
    heap_head->size = size - sizeof(heap_block_t);
    heap_head->magic = HEAP_MAGIC;
    heap_head->is_free = 1;
    heap_head->next = 0;
    heap_head->prev = 0;

    vga_print("    Heap size: ");
    vga_print_dec(size / 1024);
    vga_print(" KB\n");
}

/* Allocate memory */
void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    /* Find free block */
    heap_block_t* block = find_free_block(size);

    if (block == 0) {
        /* Expand heap - map more pages */
        uint32_t expand_size = align_size(size + sizeof(heap_block_t), PAGE_SIZE);
        uint32_t new_heap_size = heap_used + heap_free + expand_size;

        /* Map new pages */
        for (uint32_t addr = (uint32_t)heap_start + heap_used + heap_free;
             addr < (uint32_t)heap_start + new_heap_size;
             addr += PAGE_SIZE) {
            uint32_t phys = pmm_alloc_frame();
            vmm_map_page(addr, phys, PAGE_PRESENT | PAGE_WRITE);
        }

        /* Update heap block */
        heap_block_t* last = heap_head;
        while (last->next != 0) {
            last = last->next;
        }

        heap_block_t* new_block = (heap_block_t*)((uint8_t*)heap_start + heap_used + heap_free);
        new_block->size = expand_size - sizeof(heap_block_t);
        new_block->magic = HEAP_MAGIC;
        new_block->is_free = 1;
        new_block->next = 0;
        new_block->prev = last;

        last->next = new_block;

        heap_free += new_block->size + sizeof(heap_block_t);

        /* Try again */
        block = find_free_block(size);
    }

    if (block == 0) {
        vga_print("[-] Error: Out of memory!\n");
        return 0;
    }

    /* Split block if needed */
    split_block(block, size);

    /* Mark as used */
    block->is_free = 0;
    heap_used += block->size;
    heap_free -= block->size;

    return (void*)((uint8_t*)block + sizeof(heap_block_t));
}

/* Free memory */
void kfree(void* ptr) {
    if (ptr == 0) {
        return;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));

    /* Check magic */
    if (block->magic != HEAP_MAGIC) {
        vga_print("[-] Error: Invalid heap block!\n");
        return;
    }

    /* Check if already free */
    if (block->is_free) {
        vga_print("[-] Warning: Double free detected!\n");
        return;
    }

    /* Mark as free */
    block->is_free = 1;
    heap_used -= block->size;
    heap_free += block->size;

    /* Merge with adjacent blocks */
    merge_blocks(block);
}

/* Reallocate memory */
void* krealloc(void* ptr, uint32_t size) {
    if (ptr == 0) {
        return kmalloc(size);
    }

    if (size == 0) {
        kfree(ptr);
        return 0;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));

    /* Check magic */
    if (block->magic != HEAP_MAGIC) {
        return 0;
    }

    /* If new size fits in current block */
    if (block->size >= size) {
        return ptr;
    }

    /* Allocate new block */
    void* new_ptr = kmalloc(size);
    if (new_ptr == 0) {
        return 0;
    }

    /* Copy data */
    memcpy(new_ptr, ptr, block->size);

    /* Free old block */
    kfree(ptr);

    return new_ptr;
}

/* Get heap statistics */
uint32_t heap_get_total_size(void) {
    return heap_used + heap_free;
}

uint32_t heap_get_used_size(void) {
    return heap_used;
}

uint32_t heap_get_free_size(void) {
    return heap_free;
}
