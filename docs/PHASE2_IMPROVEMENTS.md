# SYNAPSE SO - Phase 2 Improvements

## Overview

This document describes advanced improvements made to Phase 2 components after Phase 3 completion. These enhancements prepare the OS for advanced features like fork(), better memory management, and improved scheduling.

**Date:** January 2025  
**Status:** DESIGN + API  
**Priority:** HIGH

---

## Improvements Implemented

### 1. Copy-on-Write (COW) Support

**Purpose:** Enable efficient fork() implementation in Phase 4

#### New API

```c
/* Clone page directory for fork() - marks pages as COW */
page_directory_t* vmm_clone_page_directory(page_directory_t* src);

/* Handle COW page fault - copies page on write */
int vmm_handle_cow_fault(uint32_t fault_addr);

/* Check if page is marked as COW */
int vmm_is_page_cow(uint32_t virt_addr);
```

#### How It Works

1. **Fork calls vmm_clone_page_directory()**
   - Parent's pages marked read-only + COW flag
   - Child gets same page mappings (shared)
   - Physical frames reference count incremented

2. **Write to COW page triggers page fault**
   - CPU detects write to read-only page
   - vmm_handle_cow_fault() checks PAGE_COW flag
   - Allocates new frame, copies data
   - Updates PTE to point to new frame, removes COW flag
   - Decrements original frame reference count

3. **Performance benefits**
   - No copying on fork (instant)
   - Memory copied only when written (lazy)
   - Shared read-only pages save memory

#### Example Usage

```c
/* Fork implementation */
process_t* fork_current_process(void) {
    process_t* parent = process_get_current();
    process_t* child = process_create(parent->name, parent->flags);
    
    /* Clone page directory with COW */
    child->page_dir = vmm_clone_page_directory(parent->page_dir);
    
    /* Copy process state */
    child->eip = parent->eip;
    child->esp = parent->esp;
    // ... copy other fields ...
    
    return child;
}

/* Page fault handler integration */
void vmm_page_fault_handler(uint32_t error_code) {
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
    
    /* Check if COW fault */
    if ((error_code & PF_WRITE) && vmm_is_page_cow(fault_addr)) {
        if (vmm_handle_cow_fault(fault_addr) == 0) {
            return;  // Handled successfully
        }
    }
    
    /* Real page fault - handle as before */
    // ...
}
```

### 2. Reference Counting in PMM

**Purpose:** Track shared pages for COW

#### New API

```c
/* Increment reference count */
void pmm_ref_frame(uint32_t frame_addr);

/* Decrement reference count, free if reaches 0 */
void pmm_unref_frame(uint32_t frame_addr);

/* Get reference count */
uint32_t pmm_get_ref_count(uint32_t frame_addr);
```

#### Implementation Details

**Data Structure:**
```c
/* Reference count table (one per frame) */
static uint16_t* frame_refcounts;  // Max 65535 refs per frame
```

**Initialization:**
```c
void pmm_init(...) {
    // Allocate refcount table
    frame_refcounts = (uint16_t*)pmm_kmalloc(num_frames * sizeof(uint16_t));
    
    // Initialize all to 0
    for (uint32_t i = 0; i < num_frames; i++) {
        frame_refcounts[i] = 0;
    }
}
```

**Usage:**
```c
/* When allocating */
uint32_t frame = pmm_alloc_frame();
pmm_ref_frame(frame);  // Initial reference

/* When sharing (COW) */
pmm_ref_frame(frame);  // Increment

/* When done with frame */
pmm_unref_frame(frame);  // Decrement, auto-free if 0
```

### 3. Priority-Based Scheduling

**Purpose:** Better responsiveness and fairness

#### Priority Levels

```c
#define PRIORITY_IDLE       0   /* Idle process only */
#define PRIORITY_LOW        1   /* Background tasks */
#define PRIORITY_NORMAL     2   /* Default user processes */
#define PRIORITY_HIGH       3   /* Interactive processes */
#define PRIORITY_REALTIME   4   /* Time-critical tasks */
```

#### New API

```c
/* Set process priority */
void scheduler_set_priority(process_t* proc, uint32_t priority);

/* Get process priority */
uint32_t scheduler_get_priority(process_t* proc);

/* Boost priority temporarily */
void scheduler_boost_priority(process_t* proc);
```

#### Scheduling Algorithm

**Multi-level feedback queue:**
```
REALTIME queue → always runs first
HIGH queue     → 4x normal quantum
NORMAL queue   → standard quantum
LOW queue      → 1/2 quantum
IDLE queue     → only when nothing else ready
```

**Priority aging:**
- Processes waiting too long get priority boost
- Prevents starvation
- Interactive processes detected and boosted

**Example:**
```c
/* Create high-priority process */
process_t* proc = process_create("important_task", PROC_FLAG_USER);
scheduler_set_priority(proc, PRIORITY_HIGH);

/* Boost priority for I/O bound process */
scheduler_boost_priority(io_process);
```

### 4. Statistics and Monitoring

#### VMM Statistics

```c
typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
    uint32_t cow_pages;       // Pages marked COW
    uint32_t shared_pages;    // Pages with refcount > 1
} vmm_stats_t;

void vmm_get_stats(vmm_stats_t* stats);
```

**Usage:**
```c
vmm_stats_t stats;
vmm_get_stats(&stats);

vga_print("VMM: ");
vga_print_dec(stats.used_pages);
vga_print(" used, ");
vga_print_dec(stats.cow_pages);
vga_print(" COW\n");
```

#### PMM Statistics

```c
typedef struct {
    uint32_t total_frames;
    uint32_t used_frames;
    uint32_t free_frames;
    uint32_t shared_frames;   // Frames with refcount > 1
} pmm_stats_t;

void pmm_get_stats(pmm_stats_t* stats);
```

#### Scheduler Statistics

```c
typedef struct {
    uint32_t total_switches;     // Total context switches
    uint32_t idle_ticks;         // Time in idle
    uint32_t busy_ticks;         // Time working
    uint32_t processes_ready;    // Processes ready to run
    uint32_t processes_blocked;  // Processes blocked
} scheduler_stats_t;

void scheduler_get_stats(scheduler_stats_t* stats);
void scheduler_reset_stats(void);
```

**Usage:**
```c
scheduler_stats_t stats;
scheduler_get_stats(&stats);

uint32_t cpu_usage = (stats.busy_ticks * 100) / 
                     (stats.busy_ticks + stats.idle_ticks);
                     
vga_print("CPU: ");
vga_print_dec(cpu_usage);
vga_print("%\n");
```

---

## Implementation Status

### API Defined ✅

All new APIs are defined in headers:
- `kernel/include/kernel/vmm.h` - COW support
- `kernel/include/kernel/pmm.h` - Reference counting
- `kernel/include/kernel/scheduler.h` - Priority scheduling

### Implementation Required ⬜

The following need to be implemented:

1. **PMM Reference Counting**
   - Allocate refcount table in `pmm_init()`
   - Implement `pmm_ref_frame()`, `pmm_unref_frame()`, `pmm_get_ref_count()`
   - Update `pmm_free_frame()` to check refcount
   - Implement `pmm_get_stats()`

2. **VMM COW Support**
   - Implement `vmm_clone_page_directory()`
   - Implement `vmm_handle_cow_fault()`
   - Implement `vmm_is_page_cow()`
   - Update `vmm_page_fault_handler()` to detect COW faults
   - Implement `vmm_get_stats()`

3. **Scheduler Priorities**
   - Add priority field to `process_t`
   - Implement multi-level queue
   - Implement `scheduler_set_priority()`, `scheduler_get_priority()`
   - Implement `scheduler_boost_priority()`
   - Implement priority aging
   - Implement `scheduler_get_stats()`, `scheduler_reset_stats()`

---

## Design Decisions

### Why COW?

**Benefits:**
- ✅ Fork is instant (no memory copy)
- ✅ Saves memory (shared pages)
- ✅ Only copies when needed (lazy)

**Trade-offs:**
- ⚠️ Page faults on first write (overhead)
- ⚠️ Complexity in page fault handler
- ⚠️ Reference counting overhead

**Verdict:** Worth it for fork() efficiency

### Why Reference Counting?

**Alternatives:**
- Garbage collection (too complex, non-deterministic)
- Manual tracking (error-prone)

**Benefits:**
- ✅ Simple to implement
- ✅ Deterministic (free immediately when count = 0)
- ✅ Low overhead (one uint16 per frame)

**Trade-offs:**
- ⚠️ Can't detect cycles (not a problem for pages)
- ⚠️ Limited to 65535 references (more than enough)

**Verdict:** Perfect for kernel use

### Why Priority Scheduling?

**Benefits:**
- ✅ Better responsiveness
- ✅ Fairer scheduling
- ✅ Prevents priority inversion

**Trade-offs:**
- ⚠️ More complex than round-robin
- ⚠️ Can cause starvation (mitigated by aging)

**Verdict:** Worth it for better user experience

---

## Performance Impact

### COW Fork

**Before (with copying):**
```
Fork 4MB process:
- Copy 1024 pages × 4KB
- Time: ~100,000 cycles
```

**After (with COW):**
```
Fork 4MB process:
- Mark pages COW: ~1,000 cycles
- Time: ~1,000 cycles (100x faster!)
```

### Reference Counting

**Overhead:**
- 2 bytes per frame = 2MB for 1GB RAM
- Increment/decrement: ~10 cycles
- Negligible compared to page operations

### Priority Scheduling

**Overhead:**
- Priority check: ~5 cycles
- Queue management: ~20 cycles per switch
- Worth it for better responsiveness

---

## Testing Plan

### COW Tests

```c
void test_cow_fork(void) {
    /* Fork process */
    process_t* parent = process_get_current();
    process_t* child = fork_current_process();
    
    /* Verify shared pages */
    uint32_t parent_phys = vmm_get_phys_addr(0x400000);
    uint32_t child_phys = vmm_get_phys_addr(0x400000);
    assert(parent_phys == child_phys);  // Same physical frame
    
    /* Verify COW flag */
    assert(vmm_is_page_cow(0x400000));
    
    /* Write triggers copy */
    *(int*)0x400000 = 42;
    child_phys = vmm_get_phys_addr(0x400000);
    assert(parent_phys != child_phys);  // Different now
    assert(!vmm_is_page_cow(0x400000)); // No longer COW
}
```

### Reference Counting Tests

```c
void test_refcounting(void) {
    uint32_t frame = pmm_alloc_frame();
    
    /* Initial reference */
    pmm_ref_frame(frame);
    assert(pmm_get_ref_count(frame) == 1);
    
    /* Add reference */
    pmm_ref_frame(frame);
    assert(pmm_get_ref_count(frame) == 2);
    
    /* Remove reference */
    pmm_unref_frame(frame);
    assert(pmm_get_ref_count(frame) == 1);
    
    /* Final unref frees frame */
    pmm_unref_frame(frame);
    // Frame should be free now
}
```

### Priority Tests

```c
void test_priority_scheduling(void) {
    /* Create processes */
    process_t* low = process_create("low", PROC_FLAG_USER);
    process_t* high = process_create("high", PROC_FLAG_USER);
    
    scheduler_set_priority(low, PRIORITY_LOW);
    scheduler_set_priority(high, PRIORITY_HIGH);
    
    /* High priority should run first */
    scheduler_tick(regs);
    assert(process_get_current() == high);
    
    /* Even if low was ready first */
}
```

---

## Migration Guide

### For Existing Code

**Old fork (hypothetical):**
```c
process_t* fork_process(process_t* parent) {
    process_t* child = process_create(...);
    
    /* Copy all pages (slow!) */
    for (each page) {
        copy_page(parent, child, addr);
    }
    
    return child;
}
```

**New fork (with COW):**
```c
process_t* fork_process(process_t* parent) {
    process_t* child = process_create(...);
    
    /* Clone with COW (fast!) */
    child->page_dir = vmm_clone_page_directory(parent->page_dir);
    
    return child;
}
```

### For New Code

**Use reference counting:**
```c
/* When sharing a frame */
uint32_t frame = vmm_get_phys_addr(addr);
pmm_ref_frame(frame);  // Increment reference

/* When done */
pmm_unref_frame(frame);  // Decrement, auto-free
```

**Use priorities:**
```c
/* Create process with priority */
process_t* proc = process_create(...);
scheduler_set_priority(proc, PRIORITY_HIGH);

/* Boost interactive processes */
if (proc_is_interactive(proc)) {
    scheduler_boost_priority(proc);
}
```

---

## Future Work

### Phase 4

1. **Implement fork() with COW**
   - Full process duplication
   - File descriptor copying
   - Signal handler copying

2. **Demand paging**
   - Allocate pages on access
   - Page-in from disk
   - Page-out when memory low

3. **Swapping**
   - Write pages to disk
   - Free memory under pressure
   - Page fault brings back from disk

### Phase 5+

1. **Transparent Huge Pages**
   - Use 2MB/4MB pages when possible
   - Reduce TLB pressure
   - Better performance

2. **NUMA Support**
   - Allocate memory on local node
   - Migrate pages for locality
   - Per-node statistics

3. **Memory Compression**
   - Compress inactive pages
   - Save memory without disk I/O
   - Transparent to applications

---

## References

- [Linux Copy-on-Write](https://en.wikipedia.org/wiki/Copy-on-write)
- [Reference Counting](https://en.wikipedia.org/wiki/Reference_counting)
- [Multi-level Feedback Queue](https://en.wikipedia.org/wiki/Multilevel_feedback_queue)
- [CONVENTIONS.md](CONVENTIONS.md) - Coding standards
- [MEMORY_MAP.md](MEMORY_MAP.md) - Memory architecture

---

**Status:** API DEFINED, IMPLEMENTATION PENDING  
**Priority:** HIGH (required for Phase 4 fork)  
**Timeline:** Implement before Phase 4 filesystem
