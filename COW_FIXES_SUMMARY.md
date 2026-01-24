# Copy-on-Write (COW) Implementation Fixes

## Overview
This document summarizes the critical fixes applied to the Copy-on-Write (COW) implementation in SYNAPSE SO to ensure proper memory isolation and prevent bugs.

## Issues Fixed

### 1. Parent Pages Not Marked as Read-Only (Importance: 10)
**File:** `kernel/vmm_cow.c` (lines 47-59)

**Problem:** Only child process pages were marked as read-only and COW. Parent pages remained writable, violating memory isolation.

**Fix:** Added line 49 to mark source PTEs as read-only and COW:
```c
/* Mark source PTE as read-only and COW */
src_pt->entries[j] = (src_pte & ~PAGE_WRITE) | PAGE_COW;
```

**Impact:** Both parent and child processes now trigger COW on write, ensuring proper memory isolation.

---

### 2. Infinite Recursion in Frame Unreferencing (Importance: 10)
**File:** `kernel/pmm_refcount.c` (lines 50-65) and `kernel/pmm.c` (lines 182-204)

**Problem:** `pmm_unref_frame()` called `pmm_free_frame()` when reference count reached 0, but `pmm_free_frame()` in turn called `pmm_unref_frame()`, causing infinite recursion.

**Fix:** Two-part fix:

**Part 1:** Removed the call to `pmm_free_frame()` from `pmm_unref_frame()`:
```c
/* Decrement reference count if greater than 0 */
if (frame_refcounts[frame_num] > 0) {
    frame_refcounts[frame_num]--;
}
```

**Part 2:** Updated `pmm_free_frame()` to check reference count BEFORE decrementing:
```c
/* Check reference count before decrementing */
uint32_t refcount = pmm_get_ref_count(frame_addr);
if (refcount == 0U) {
    /* Reference count already 0, should not happen */
    return;
}

if (refcount == 1U) {
    /* This is the last reference, free the frame */
    pmm_unref_frame(frame_addr);
    frame_set_free(frame);
} else {
    /* There are other references, just decrement the count */
    pmm_unref_frame(frame_addr);
}
```

**Impact:** Eliminates stack overflow from infinite recursion. Frames are only marked as free when the last reference is released.

---

### 3. PAGE_COW Flag Incorrectly Applied to PDE (Importance: 9)
**File:** `kernel/vmm_cow.c` (lines 61-69)

**Problem:** The PAGE_COW flag was being applied to Page Directory Entries (PDEs), but it should only be applied to Page Table Entries (PTEs).

**Fix:** Changed line 62 to remove PAGE_COW and preserve correct flags:
```c
/* Set new page directory entry */
new_dir->entries[i] = new_pt_phys | (src_pde & 0xFFF) | PAGE_PRESENT | PAGE_USER;
```

Also added kernel mappings copy (lines 66-69):
```c
/* Copy kernel mappings (PDE 768-1023 = kernel space at 3GB+) */
for (uint32_t i = 768U; i < 1024U; i++) {
    new_dir->entries[i] = src->entries[i];
}
```

**Impact:** Correct PDE flags ensure child processes can access kernel functions. Kernel mappings are properly inherited.

---

### 4. Unsafe Memory Copying in COW Fault Handler (Importance: 8)
**File:** `kernel/vmm_cow.c` (lines 86-125)

**Problem:** The COW fault handler assumed identity mapping (`phys_addr + KERNEL_VIRT_START`) to copy page data, which is unsafe and may not work correctly.

**Fix:** Replaced direct memory access with temporary mappings (lines 93-115):
```c
/* Allocate temporary mapping slots */
int temp_slot_src = vmm_alloc_temp_slot();
int temp_slot_dest = vmm_alloc_temp_slot();

if (temp_slot_src < 0) {
    vga_print("[-] Failed to allocate temp slot for COW\n");
    pmm_free_frame(new_phys);
    return -1;
}

if (temp_slot_dest < 0) {
    vga_print("[-] Failed to allocate temp slot for COW\n");
    vmm_free_temp_slot(temp_slot_src);
    pmm_free_frame(new_phys);
    return -1;
}

/* Temporarily map pages to copy data */
uint32_t temp_virt_src = vmm_map_temp_page(original_phys, temp_slot_src);
uint32_t temp_virt_dest = vmm_map_temp_page(new_phys, temp_slot_dest);

/* Copy data from original page to new page */
/* Note: Both source and destination are properly mapped pages of PAGE_SIZE (4096 bytes),
 * so copying PAGE_SIZE bytes is safe and will not overflow */
memcpy((void*)temp_virt_dest, (void*)temp_virt_src, PAGE_SIZE);

/* Unmap temporary pages */
vmm_unmap_temp_page(temp_slot_src);
vmm_unmap_temp_page(temp_slot_dest);
vmm_free_temp_slot(temp_slot_src);
vmm_free_temp_slot(temp_slot_dest);
```

**Impact:** Safer and more robust page copying that works correctly regardless of address space layout.

---

### 5. Reference Counts Not Initialized for Used Frames (Importance: 9)
**File:** `kernel/pmm.c` (lines 137-145)

**Problem:** After `pmm_refcount_init()` set all reference counts to 0, frames already marked as in-use still had refcount=0, potentially causing premature deallocation.

**Fix:** Added initialization loop after `pmm_refcount_init()`:
```c
/* Initialize reference counting */
pmm_refcount_init(total_frames);

/* Set initial refcount for all used frames */
for (uint32_t f = 0; f < total_frames; f++) {
    if (!frame_is_free(f)) {
        pmm_ref_frame(frame_to_addr(f));
    }
}
```

**Impact:** All in-use frames now correctly start with refcount=1, preventing premature deallocation.

---

### 6. PTE Flags Lost During COW Fault Handling (Importance: 7)
**File:** `kernel/vmm_cow.c` (line 118)

**Problem:** When handling a COW fault, the PTE was updated with hardcoded flags (`PAGE_PRESENT | PAGE_WRITE | PAGE_USER`), losing any other flags that might have been set.

**Fix:** Preserve existing flags while clearing PAGE_COW and adding PAGE_WRITE:
```c
/* Preserve flags when updating PTE */
uint32_t flags = (*pte & ~(PAGE_COW | PAGE_WRITE)) | PAGE_WRITE;
*pte = new_phys | flags;
```

**Impact:** All page attributes are preserved during COW copy, maintaining correct memory permissions and attributes.

---

### 7. MISRA C Compliance Improvements (Importance: 8)
**File:** `kernel/vmm_cow.c` and `kernel/pmm.c`

**Problem:** Several MISRA C rule violations detected by static analysis:
- Unsigned integer comparisons with magic numbers (768, 1024)
- Complex conditional expressions in single if statements
- Missing bounds checking documentation

**Fixes Applied:**
1. Added `U` suffix to all loop limit constants (768U, 1024U)
2. Split complex conditional into separate if statements
3. Added explicit `refcount == 0U` check before `refcount == 1U`
4. Added documentation explaining memcpy bounds safety
5. Removed unnecessary blank lines

**Impact:** Code is now more compliant with MISRA C rules and easier to audit.

---

## Summary

All seven fixes have been successfully applied:

1. ✅ Parent pages marked as read-only and COW
2. ✅ Infinite recursion in frame unreferencing eliminated
3. ✅ PAGE_COW removed from PDE, kernel mappings copied
4. ✅ Temporary mappings used for safe COW page copying
5. ✅ Reference counts initialized for used frames
6. ✅ PTE flags preserved during COW fault handling
7. ✅ MISRA C compliance improvements

## Testing

The changes should be tested with:
1. Fork system calls to verify COW behavior
2. Write operations in both parent and child processes
3. Memory allocation and deallocation stress tests
4. Reference counting verification under heavy load
5. Static analysis with MISRA C compliance checking

## Files Modified

- `kernel/vmm_cow.c` - COW implementation with MISRA fixes
- `kernel/pmm_refcount.c` - Reference counting
- `kernel/pmm.c` - Physical memory manager with MISRA fixes

## Backward Compatibility

These changes maintain backward compatibility with existing code while fixing critical bugs in the COW implementation.

## Code Quality Improvements

- Added unsigned suffixes to constants (768U, 1024U) for MISRA compliance
- Split complex conditionals into separate checks for clarity
- Added documentation for memcpy bounds safety
- Proper variable types (int vs int32_t consistency)
- Removed trailing whitespace and blank lines
