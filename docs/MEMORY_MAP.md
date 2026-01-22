# SYNAPSE SO - Memory Map

## Physical Memory Layout

### Overview

SYNAPSE SO uses a 32-bit address space with paging enabled. Physical memory is divided into frames of 4KB each.

### Physical Address Space

```
┌─────────────────────────────────────┬─────────────┬────────────────────────┐
│ Range                               │ Size        │ Purpose                │
├─────────────────────────────────────┼─────────────┼────────────────────────┤
│ 0x00000000 - 0x000003FF             │ 1 KB        │ Real Mode IVT          │
│ 0x00000400 - 0x000004FF             │ 256 B       │ BIOS Data Area (BDA)   │
│ 0x00000500 - 0x00007BFF             │ ~30 KB      │ Usable (not used)      │
│ 0x00007C00 - 0x00007DFF             │ 512 B       │ Bootloader (historical)│
│ 0x00007E00 - 0x0007FFFF             │ ~480 KB     │ Usable (not used)      │
│ 0x00080000 - 0x0009FFFF             │ 128 KB      │ Extended BDA           │
│ 0x000A0000 - 0x000BFFFF             │ 128 KB      │ Video memory           │
│ 0x000C0000 - 0x000FFFFF             │ 256 KB      │ ROM, BIOS              │
├─────────────────────────────────────┼─────────────┼────────────────────────┤
│ 0x00100000 - 0x001FFFFF             │ 1 MB        │ Kernel image           │
│ 0x00200000 - 0x002FFFFF             │ 1 MB        │ Kernel data & bitmap   │
│ 0x00300000 - 0x003FFFFF             │ 1 MB        │ Kernel heap (initial)  │
│ 0x00400000 - [RAM END]              │ Varies      │ Available frames       │
└─────────────────────────────────────┴─────────────┴────────────────────────┘
```

### Reserved Regions (0x0 - 0x100000)

This 1MB region is reserved for BIOS and legacy compatibility:

- **IVT (Interrupt Vector Table)**: Real-mode interrupt vectors
- **BDA (BIOS Data Area)**: BIOS variables and state
- **Video Memory**: VGA text mode buffer at `0xB8000`
- **ROM**: BIOS code and option ROMs

**Note:** We identity-map this region to maintain access to VGA buffer.

### Kernel Region (0x100000 - 0x400000)

The kernel occupies the first 3MB above 1MB:

#### Kernel Image (0x100000 - 0x1FFFFF)
- `.text` section: Kernel code
- `.rodata` section: Read-only data (strings, constants)
- `.data` section: Initialized global variables
- `.bss` section: Uninitialized global variables

#### PMM Bitmap (0x200000 - 0x2FFFFF)
- Physical Memory Manager frame bitmap
- 1 bit per frame (4KB)
- Can track up to 32GB of RAM (8MB frames × 4KB)

#### Initial Heap (0x300000 - 0x3FFFFF)
- Temporary heap before paging
- Used during early initialization
- Later replaced by proper heap allocator

### Available Frames (0x400000+)

All physical memory above 4MB is available for allocation:

- Process page directories and tables
- Process user memory
- Kernel heap expansion
- DMA buffers
- Memory-mapped devices

## Virtual Memory Layout

### Overview

With paging enabled, the virtual address space is divided into user and kernel regions:

```
┌─────────────────────────────────────┬─────────────┬────────────────────────┐
│ Virtual Range                       │ Size        │ Purpose                │
├─────────────────────────────────────┼─────────────┼────────────────────────┤
│ 0x00000000 - 0x000FFFFF             │ 1 MB        │ Identity mapped (BIOS) │
│ 0x00100000 - 0x003FFFFF             │ 3 MB        │ Identity mapped (kern) │
│ 0x00400000 - 0x3FFFFFFF             │ ~1 GB       │ User space             │
│ 0x40000000 - 0xBFFFFFFF             │ 2 GB        │ Reserved (future)      │
├─────────────────────────────────────┼─────────────┼────────────────────────┤
│ 0xC0000000 - 0xC00FFFFF             │ 1 MB        │ Kernel image (higher)  │
│ 0xC0100000 - 0xDFFFFFFF             │ ~510 MB     │ Kernel heap            │
│ 0xE0000000 - 0xE00FFFFF             │ 1 MB        │ Temporary mappings     │
│ 0xE0100000 - 0xFFFFFFFF             │ ~495 MB     │ Reserved (future)      │
└─────────────────────────────────────┴─────────────┴────────────────────────┘
```

### Identity Mapped Region (0x0 - 0x400000)

The first 4MB are identity-mapped (virtual = physical):

- **Purpose:** Access low memory and VGA buffer
- **Permissions:** Kernel read/write
- **Note:** Required for VGA driver at `0xB8000`

### User Space (0x0 - 0x3FFFFFFF)

The first 1GB is designated for user processes:

**Typical Process Layout:**
```
0x00000000        ┌─────────────────────┐
                  │ NULL guard page     │  ← Invalid, catches null pointer derefs
0x00001000        ├─────────────────────┤
                  │ .text (code)        │  ← ELF program segments
                  │ .rodata (readonly)  │
                  │ .data (initialized) │
                  │ .bss (zeroed)       │
0x08000000        ├─────────────────────┤
                  │ Heap (grows up ↑)   │  ← malloc() allocations
                  │         ...         │
                  ├─────────────────────┤
                  │         ...         │
                  │         ...         │
0x3FFFF000        ├─────────────────────┤
                  │ Stack (grows down ↓)│  ← Local variables, call frames
0x40000000        └─────────────────────┘
```

**Characteristics:**
- Each process has its own page directory
- User-mode accessible (`PAGE_USER` flag)
- Process isolation via virtual memory

### Kernel Space (0xC0000000 - 0xFFFFFFFF)

The upper 1GB is reserved for the kernel (higher-half kernel):

#### Kernel Image (0xC0000000 - 0xC00FFFFF)

Maps to physical `0x100000 - 0x1FFFFF`:

```
Virtual: 0xC0000000 + offset → Physical: 0x00100000 + offset
```

**Sections:**
- `.text`: Kernel code (execute, read-only)
- `.rodata`: Constants (read-only)
- `.data`: Initialized variables (read/write)
- `.bss`: Uninitialized variables (read/write)

#### Kernel Heap (0xC0100000 - 0xDFFFFFFF)

Dynamic kernel memory allocation:

- **Size:** ~510 MB (expandable on demand)
- **Allocator:** Free-list based
- **Functions:** `kmalloc()`, `kfree()`, `krealloc()`
- **Features:**
  - Block splitting and coalescing
  - Automatic expansion via VMM
  - Alignment support

**Typical Allocations:**
- Process control blocks (PCB)
- Page tables
- Driver buffers
- String buffers

#### Temporary Mappings (0xE0000000 - 0xE00FFFFF)

Special region for temporary frame mappings:

- **Size:** 1 MB (256 pages)
- **Purpose:** Access process memory from kernel
- **Use Cases:**
  - System call parameter validation
  - Copying data between address spaces
  - Reading/writing user buffers safely

**API:**
```c
int slot = vmm_alloc_temp_slot();           // Allocate slot
uint32_t virt = vmm_map_temp_page(phys, slot);  // Map physical frame
// ... access memory at virt ...
vmm_unmap_temp_page(slot);                  // Unmap (no frame free)
vmm_free_temp_slot(slot);                   // Free slot
```

**Implementation Details:**
- Bitmap tracks slot allocation
- Always mapped to kernel page directory
- Does NOT free physical frame on unmap
- Thread-safe slot allocation (critical)

## Page Table Structure

### Two-Level Paging

SYNAPSE SO uses x86 two-level paging:

```
Virtual Address (32 bits):
┌──────────┬──────────┬──────────────┐
│   10     │    10    │      12      │
│ PD Index │ PT Index │ Page Offset  │
└──────────┴──────────┴──────────────┘
   Bits     Bits        Bits
  31..22    21..12      11..0

CR3 Register:
┌─────────────────────────────────────┐
│ Physical address of Page Directory  │
└─────────────────────────────────────┘

Page Directory Entry (PDE):
┌──────────────────┬─────┬─────────────┐
│  Page Table      │Flags│   Reserved  │
│  Physical Address│     │             │
└──────────────────┴─────┴─────────────┘
  Bits 31..12      11..0

Page Table Entry (PTE):
┌──────────────────┬─────┬─────────────┐
│  Physical Frame  │Flags│   Reserved  │
│  Address         │     │             │
└──────────────────┴─────┴─────────────┘
  Bits 31..12      11..0
```

### Page Flags

Both PDE and PTE use the same flag bits:

```c
#define PAGE_PRESENT    (1 << 0)  // Page is in memory
#define PAGE_WRITE      (1 << 1)  // Page is writable
#define PAGE_USER       (1 << 2)  // User-mode accessible
#define PAGE_WRITETHROUGH (1 << 3)  // Write-through caching
#define PAGE_NOCACHE    (1 << 4)  // Disable caching
#define PAGE_ACCESSED   (1 << 5)  // Accessed (set by CPU)
#define PAGE_DIRTY      (1 << 6)  // Written (set by CPU)
#define PAGE_GLOBAL     (1 << 8)  // Global (TLB) page
```

**Common Combinations:**
- **Kernel code:** `PAGE_PRESENT`
- **Kernel data:** `PAGE_PRESENT | PAGE_WRITE`
- **User code:** `PAGE_PRESENT | PAGE_USER`
- **User data:** `PAGE_PRESENT | PAGE_WRITE | PAGE_USER`

### Address Translation Example

Translate virtual address `0xC0101234` to physical:

1. **Extract indices:**
   - PD index: `0xC0101234 >> 22 = 768` (0x300)
   - PT index: `(0xC0101234 >> 12) & 0x3FF = 257` (0x101)
   - Offset: `0xC0101234 & 0xFFF = 0x234`

2. **Walk page tables:**
   - Load CR3 → Page Directory physical address
   - Index into PD[768] → Get Page Table physical address
   - Index into PT[257] → Get physical frame address
   - Add offset: `frame + 0x234` = final physical address

3. **Result:**
   - Virtual: `0xC0101234`
   - Physical: `0x00101234` (assuming identity offset)

## Memory Allocation Strategies

### Physical Memory (PMM)

**Frame Allocation Algorithm:**
1. Search bitmap for first free frame
2. Mark frame as used
3. Return physical address

**Fragmentation:**
- External fragmentation possible
- Mitigated by 4KB granularity
- Future: Buddy allocator for large blocks

### Virtual Memory (VMM)

**Page Mapping:**
1. Allocate physical frame (PMM)
2. Find/create page table
3. Set PTE with frame address and flags
4. Flush TLB for virtual address

**Page Tables:**
- Allocated on-demand
- Shared kernel mappings (PD entries 768-1023)
- Each process has unique user mappings

### Kernel Heap

**Allocation Strategy:**
1. Search free list for suitable block
2. Split block if too large
3. Mark block as used
4. Return pointer

**Deallocation Strategy:**
1. Mark block as free
2. Coalesce with adjacent free blocks
3. Return to free list

## Memory Protection

### Privilege Levels

- **Ring 0 (Kernel):** Full access to all memory
- **Ring 3 (User):** Access only to pages with `PAGE_USER` flag

### Protection Mechanisms

1. **Page Permissions:**
   - Write protection (`PAGE_WRITE` flag)
   - User/kernel separation (`PAGE_USER` flag)
   - Execute disable (future: NX bit)

2. **Page Fault Handling:**
   - Detects invalid accesses
   - Error code indicates cause:
     - Bit 0: Page present
     - Bit 1: Write operation
     - Bit 2: User mode
     - Bit 3: Reserved bit violation
     - Bit 4: Instruction fetch

3. **Address Space Isolation:**
   - Each process has separate page directory
   - User processes cannot access kernel memory (without syscall)
   - User processes cannot access other processes' memory

## Memory Statistics

### Available via PMM

```c
uint32_t pmm_get_free_frames(void);   // Free frames
uint32_t pmm_get_used_frames(void);   // Used frames
uint32_t pmm_get_total_frames(void);  // Total frames
```

### Available via Heap

```c
heap_stats_t stats = heap_get_stats();
// stats.total_size
// stats.used_size
// stats.free_size
// stats.num_allocations
```

## Future Enhancements

### Phase 3+

- **Swapping:** Move pages to disk when memory is low
- **Copy-on-Write:** Efficient fork() implementation
- **Shared Memory:** IPC via shared pages
- **Memory-Mapped Files:** Map files into address space
- **Large Pages:** 4MB pages for performance
- **NUMA Support:** Non-uniform memory access

### Security

- **ASLR:** Address Space Layout Randomization
- **DEP/NX:** Data Execution Prevention
- **SMEP/SMAP:** Supervisor Mode Execution/Access Prevention
- **Stack Guard Pages:** Detect stack overflow

## References

- Intel® 64 and IA-32 Architectures Software Developer's Manual, Volume 3
- OSDev Wiki: [Paging](https://wiki.osdev.org/Paging)
- [TECHNICAL_REFERENCE.md](TECHNICAL_REFERENCE.md) - Implementation details
