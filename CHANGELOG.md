# SYNAPSE SO - Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Security
- **CRITICAL**: Fixed double-free vulnerability in `vmm_unmap_temp_page()` ([#1])
- **CRITICAL**: Fixed user pointer dereference without validation in `sys_write()` ([#4])
- **HIGH**: Fixed page directory confusion in temporary mappings ([#2])
- **HIGH**: Fixed race conditions in temporary slot allocation ([#3])
- **MEDIUM**: Documented physical address resolution limitations ([#5])

### Added
- New unified build script `build.sh` with Docker support
- Comprehensive documentation:
  - `docs/BOOT_PROCESS.md` - Complete boot sequence guide
  - `docs/MEMORY_MAP.md` - Memory architecture documentation
  - `docs/CONVENTIONS.md` - Calling conventions and standards
  - `docs/TESTING.md` - Testing guide and best practices
  - `docs/PHASE2_CRITICAL_FIXES.md` - Security fixes documentation
- `IMPROVEMENTS_SUMMARY.md` - Summary of all improvements
- New VMM functions:
  - `vmm_unmap_page_no_free()` - Unmap without freeing physical frame
  - `vmm_alloc_temp_slot()` - Bitmap-based slot allocator
  - `vmm_free_temp_slot()` - Free temporary slot
- Enhanced `CONTRIBUTING.md` with conventional commit examples

### Changed
- **BREAKING**: Temporary mapping API redesigned from address-based to slot-based
  - Old: `vmm_map_temp_page(phys)` / `vmm_unmap_temp_page(virt)`
  - New: `vmm_alloc_temp_slot()` + `vmm_map_temp_page(phys, slot)` + `vmm_unmap_temp_page(slot)` + `vmm_free_temp_slot(slot)`
- `sys_write()` completely rewritten for security:
  - Added user pointer validation
  - Added page boundary handling
  - Added DOS protection (4KB limit)
  - Uses temporary mappings for safe access
- `vmm_map_temp_page()` now always maps to kernel directory
- `vmm_unmap_temp_page()` no longer frees physical frames

### Fixed
- Fixed TAB/space issues in Makefile causing build failures
- Fixed memory corruption from double-free in temporary unmapping
- Fixed race conditions in temporary slot allocation
- Fixed kernel crashes from invalid user pointers
- Fixed security vulnerabilities in system call handling

## [0.2.0] - 2025-01 - Phase 2 Completion

### Added
- Physical Memory Manager (PMM) with bitmap allocation
- Virtual Memory Manager (VMM) with paging support
- Kernel heap allocator (kmalloc/kfree/krealloc)
- Process management (PCB, process states, PID assignment)
- Round-robin scheduler with time quantum
- ELF32 loader with validation
- Context switching (assembly routines)
- System call interface (INT 0x80, Linux-style)
- Timer driver (PIT, 100Hz)
- Extended string library (memcpy, memset, strncpy, strncmp)
- Page fault handler with detailed error reporting

### Changed
- Kernel now runs in higher half (3GB+)
- Memory layout reorganized for user/kernel separation
- Interrupts properly handled via IDT

### Fixed
- CR3 address calculation bugs
- ELF buffer validation issues
- Page directory management in ELF loader
- Allocation failure handling

## [0.1.0] - 2024-12 - Phase 1 Completion

### Added
- Multiboot-compliant bootloader
- VGA text mode driver (80x25, 16 colors)
- Global Descriptor Table (GDT) setup
- Interrupt Descriptor Table (IDT) setup
- Basic kernel initialization
- Build system with Makefile
- Initial documentation

### Documentation
- README.md
- ARCHITECTURE.md
- ROADMAP.md
- QUICKSTART.md
- DEVELOPMENT.md

---

## Issue References

- [#1]: Double-free in temporary mappings
- [#2]: Wrong page directory for temporary mappings
- [#3]: Unsynchronized temporary slot allocation
- [#4]: User pointer dereference without validation
- [#5]: Physical address resolution in wrong context

---

## Migration Guide

### Temporary Mapping API (0.2.x → 0.3.0)

**Old Code:**
```c
uint32_t virt = vmm_map_temp_page(phys_addr);
// ... use mapping ...
vmm_unmap_temp_page(virt);
```

**New Code:**
```c
int slot = vmm_alloc_temp_slot();
if (slot < 0) {
    // Handle error: no slots available
    return -1;
}

uint32_t virt = vmm_map_temp_page(phys_addr, slot);
if (virt == 0) {
    vmm_free_temp_slot(slot);
    // Handle error: mapping failed
    return -1;
}

// ... use mapping ...

vmm_unmap_temp_page(slot);  // Does NOT free physical frame
vmm_free_temp_slot(slot);

// If you own the physical frame, free it manually:
pmm_free_frame(phys_addr);
```

**Key Differences:**
1. Must allocate slot before mapping
2. Must free slot after unmapping
3. Physical frame is NOT freed automatically
4. Explicit error handling at each step

---

## Compatibility

### Supported Architectures
- x86 (32-bit)

### Build Requirements
- GCC 4.8+ with multilib support
- NASM 2.15+
- GNU Binutils 2.25+
- GRUB 2.0+
- QEMU 2.0+ (for testing)

### Tested Environments
- Ubuntu 20.04+
- Debian 11+
- Fedora 35+
- Arch Linux (current)
- Docker (Ubuntu 22.04 base)

---

## Known Issues

### Phase 2
- [ ] Temporary slot allocator not SMP-safe (requires spinlock)
- [ ] No cross-address-space copy function yet
- [ ] Timer not connected to scheduler (scheduler_tick not called)
- [ ] No user mode processes yet (kernel mode only)

### Future
- [ ] No filesystem support
- [ ] No network stack
- [ ] No graphics mode (VGA text only)
- [ ] No multiprocessor support (SMP)

---

## Contributors

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

**Legend:**
- 🔒 Security fix
- ⚠️ Breaking change
- ✨ New feature
- 🐛 Bug fix
- 📚 Documentation
- 🔧 Internal change
