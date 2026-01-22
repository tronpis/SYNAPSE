# Critical Fix: Temporary Mapping Page Fault Issue

## Issue Summary

**Severity:** CRITICAL 🚨  
**Status:** FIXED  
**Date:** January 2025

## The Bug

The initial implementation of temporary page mappings caused **page faults** when accessed from system calls.

### Root Cause

```
1. vmm_create_page_directory() copies kernel mappings (PD entries 768-1023)
   from kernel_directory at CREATION TIME

2. Later, vmm_map_temp_page() creates NEW page table in kernel_directory

3. This new page table is NOT in process's page directory
   (it was copied before the new page table was created)

4. sys_write() runs with process CR3 → accesses temp_virt → PAGE FAULT
   (because temp_virt is not mapped in the active page directory)
```

### Visual Explanation

```
Time: T0 (Process Creation)
┌─────────────────────────────────────┐
│ kernel_directory (PD)               │
│  Entry 768-1023: Kernel mappings    │
│  Entry 896 (0xE0000000): [EMPTY]    │◄─── No temp mapping yet
└─────────────────────────────────────┘
              │
              │ Copy entries 768-1023
              ▼
┌─────────────────────────────────────┐
│ process_directory (PD)              │
│  Entry 768-1023: Kernel mappings    │
│  Entry 896 (0xE0000000): [EMPTY]    │◄─── Copied empty
└─────────────────────────────────────┘

Time: T1 (Syscall - OLD BROKEN CODE)
┌─────────────────────────────────────┐
│ kernel_directory (PD)               │
│  Entry 896: [NEW PAGE TABLE]        │◄─── Mapping created here
└─────────────────────────────────────┘
              ✗ 
              NOT reflected in process!
              ✗
┌─────────────────────────────────────┐
│ process_directory (PD)              │
│  Entry 896: [STILL EMPTY]           │◄─── Process CR3 active
└─────────────────────────────────────┘
              │
              │ sys_write() accesses temp_virt
              ▼
         PAGE FAULT! 💥
```

### The Broken Code

```c
/* BROKEN - Maps to kernel_directory */
uint32_t vmm_map_temp_page(uint32_t phys_addr, int slot) {
    uint32_t virt_addr = TEMP_MAPPING_BASE + (slot * PAGE_SIZE);
    
    page_directory_t* old_dir = current_directory;
    current_directory = kernel_directory;  // ← Switch away from process dir
    
    vmm_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);  // ← Maps here
    
    current_directory = old_dir;  // ← Switch back
    return virt_addr;  // ← Returns address that's NOT in process CR3!
}
```

**Result:** When `sys_write()` dereferences `temp_virt`, the page is not present in the active CR3 (process directory), causing a page fault.

## The Fix

Map to the **currently active directory** (the one in CR3):

```c
/* FIXED - Maps to current_directory (active CR3) */
uint32_t vmm_map_temp_page(uint32_t phys_addr, int slot) {
    if (slot < 0 || slot >= TEMP_MAPPING_PAGES) {
        return 0;
    }
    
    uint32_t virt_addr = TEMP_MAPPING_BASE + (slot * PAGE_SIZE);
    
    /* Map in the currently active directory
     * No switching - map where CR3 is pointing RIGHT NOW
     */
    vmm_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
    
    return virt_addr;
}
```

### Why This Works

1. **Syscalls execute in kernel mode** even when CR3 points to a process directory
2. **Kernel-only mapping** (no `PAGE_USER` flag) prevents user mode access
3. **Mapping is in active CR3** so kernel mode can access it immediately
4. **Temporary nature** - mapping only needs to exist during the syscall

### Visual Explanation (Fixed)

```
Time: T1 (Syscall - NEW FIXED CODE)
┌─────────────────────────────────────┐
│ process_directory (PD)              │◄─── CR3 points here
│  Entry 896: [NEW PAGE TABLE]        │◄─── Mapping created HERE
└─────────────────────────────────────┘
              │
              │ sys_write() accesses temp_virt
              ▼
         SUCCESS! ✅
    (Mapping exists in active CR3)
```

## Security Considerations

### Is it safe to map in process directory?

**Yes!** Here's why:

1. **Privilege separation works**
   - Mapping uses kernel-only flags: `PAGE_PRESENT | PAGE_WRITE`
   - No `PAGE_USER` flag set
   - User mode cannot access the temporary region (0xE0000000+)
   - Only kernel mode (syscall execution) can access it

2. **Address space isolation**
   - Temp region is in kernel space (above 0xC0000000)
   - Each process has its own temp mappings
   - Mappings are cleaned up after syscall
   - No cross-process leakage

3. **Attack scenarios prevented**
   - User mode tries to access 0xE0000000 → Page fault (no PAGE_USER)
   - User mode tries to map PAGE_USER pages there → Denied (kernel-only region)
   - One process can't see another's temp mappings → Isolated page directories

### Test: User Mode Access

```c
/* From user mode */
void* temp_addr = (void*)0xE0000000;
char c = *(char*)temp_addr;  // PAGE FAULT - no PAGE_USER flag
```

## Impact

### Before Fix
- ❌ System calls using temp mappings would page fault
- ❌ `sys_write()` couldn't access user buffers safely
- ❌ ELF loading might fail with page faults
- ❌ Any kernel code using temp mappings would crash

### After Fix
- ✅ System calls can safely access user memory
- ✅ Temp mappings work as expected
- ✅ No page faults from temp mapping access
- ✅ Proper isolation and security maintained

## Testing

### Test Case 1: sys_write() with User Buffer

```c
/* From user mode */
char buffer[100] = "Hello, kernel!";
write(1, buffer, 14);  // Triggers sys_write()

/* Expected: No page fault, message printed */
```

### Test Case 2: Cross-Page Buffer

```c
/* From user mode */
char* buf = mmap(0x1000, 8192);  // Allocate 2 pages
memset(buf, 'A', 8192);
write(1, buf + 4000, 100);  // Crosses page boundary

/* Expected: No page fault, handles boundary correctly */
```

### Test Case 3: Invalid Pointer

```c
/* From user mode */
write(1, (void*)0xDEADBEEF, 100);  // Invalid address

/* Expected: Returns -1, no kernel crash */
```

## Code Changes

### Files Modified

1. **kernel/vmm.c**
   - Removed `current_directory` switching in `vmm_map_temp_page()`
   - Removed `current_directory` switching in `vmm_unmap_temp_page()`
   - Added comments explaining why this works

2. **docs/PHASE2_CRITICAL_FIXES.md**
   - Updated Bug #2 description with correct explanation
   - Added visual diagrams
   - Explained security model

### Diff Summary

```diff
 uint32_t vmm_map_temp_page(uint32_t phys_addr, int slot) {
-    page_directory_t* old_dir = current_directory;
-    current_directory = kernel_directory;
     vmm_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
-    current_directory = old_dir;
     return virt_addr;
 }

 void vmm_unmap_temp_page(int slot) {
-    page_directory_t* old_dir = current_directory;
-    current_directory = kernel_directory;
     vmm_unmap_page_no_free(virt_addr);
-    current_directory = old_dir;
 }
```

## Lessons Learned

1. **Page directory copying is one-time**
   - Changes to `kernel_directory` after process creation don't propagate
   - Must map to active directory or pre-allocate all kernel page tables

2. **Kernel mode vs user mode is privilege, not address space**
   - Kernel mode can access kernel-only pages even in process CR3
   - Address space isolation works at page level (PAGE_USER flag)

3. **Temporary mappings are per-process**
   - Each process needs its own temp mappings
   - Mapping to `kernel_directory` doesn't help processes

4. **CR3 points to the active directory**
   - All memory accesses go through the directory in CR3
   - `current_directory` is just a variable, not what CPU uses

## Future Work

For Phase 3+, consider:

1. **Pre-allocate temp page tables** in `kernel_directory` before creating processes
2. **Add TLB invalidation** for SMP (when temp mapping changes)
3. **Add spinlocks** for SMP-safe slot allocation
4. **Track temp mappings per-process** for debugging

## References

- Intel® 64 and IA-32 Architectures Software Developer's Manual, Vol. 3A, Section 4.3 (Paging)
- Linux kernel: `kmap()` implementation (similar concept)
- OSDev Wiki: [Higher Half Kernel](https://wiki.osdev.org/Higher_Half_x86_Bare_Bones)

---

**Status:** ✅ FIXED  
**Verified:** Manually tested with sys_write()  
**Review:** Passed code review by external reviewer
