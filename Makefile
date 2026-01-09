# SYNAPSE SO - Makefile
# Licensed under GPLv3

# ============================================================================
# REQUIREMENTS
# ============================================================================
# This build system requires the following tools:
# - gcc with multilib support (for -m32 flag)
#   Ubuntu/Debian: sudo apt-get install gcc-multilib
# - nasm assembler (version 2.15+)
#   Ubuntu/Debian: sudo apt-get install nasm
# - GNU ld linker (from binutils)
#   Ubuntu/Debian: sudo apt-get install binutils
# - grub-mkrescue (for ISO generation)
#   Ubuntu/Debian: sudo apt-get install grub-pc-bin xorriso
# - qemu-system-x86_64 (for testing, optional)
#   Ubuntu/Debian: sudo apt-get install qemu-system-x86
#
# Test availability with: which gcc nasm ld grub-mkrescue qemu-system-x86_64

# ============================================================================
# TOOLS
# ============================================================================
CC = gcc
AS = nasm
LD = ld
GRUB_MKRESCUE = grub-mkrescue

# ============================================================================
# COMPILER FLAGS
# ============================================================================
# -m32: Compile as 32-bit
# -ffreestanding: No standard library
# -nostdlib: Don't link standard library
# -fno-stack-protector: No stack protector (kernel code)
# -fno-pie: No position-independent executable
# -Wall -Wextra: Extra warnings
# -O2: Optimization level 2
CFLAGS = -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pie -Wall -Wextra -O2

# -m elf_i386: Link as 32-bit ELF
# -T boot/linker.ld: Use kernel linker script
LDFLAGS = -m elf_i386 -T boot/linker.ld

# -f elf32: Output 32-bit ELF
ASFLAGS = -f elf32

# ============================================================================
# DIRECTORIES
# ============================================================================
KERNEL_DIR = kernel
BOOT_DIR = boot
BUILD_DIR = build
ISO_DIR = isodir

# ============================================================================
# SOURCE FILES
# ============================================================================
# Boot assembly
BOOT_ASM = $(BOOT_DIR)/boot.asm

# Kernel assembly files
KERNEL_ASM = $(KERNEL_DIR)/isr.asm \
	$(KERNEL_DIR)/switch.asm

# Kernel C source files (explicit list to avoid pattern conflicts)
KERNEL_C_FILES = $(KERNEL_DIR)/kernel.c \
	$(KERNEL_DIR)/vga.c \
	$(KERNEL_DIR)/gdt.c \
	$(KERNEL_DIR)/idt.c \
	$(KERNEL_DIR)/pmm.c \
	$(KERNEL_DIR)/vmm.c \
	$(KERNEL_DIR)/heap.c \
	$(KERNEL_DIR)/process.c \
	$(KERNEL_DIR)/scheduler.c \
	$(KERNEL_DIR)/timer.c \
	$(KERNEL_DIR)/elf.c

# Library C source files
KERNEL_LIB_FILES = $(KERNEL_DIR)/lib/string.c
KERNEL_C_OBJS := $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C_FILES))
KERNEL_LIB_OBJS := $(patsubst $(KERNEL_DIR)/lib/%.c,$(BUILD_DIR)/%.o,$(KERNEL_LIB_FILES))

# ============================================================================
# OUTPUT FILES
# ============================================================================
KERNEL_BIN = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = synapse.iso

# ============================================================================
# TARGETS
# ============================================================================

# Default target
all: $(ISO_IMAGE)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# ============================================================================
# BOOT CODE
# ============================================================================

# Compile boot assembly
$(BUILD_DIR)/boot.o: $(BOOT_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# ============================================================================
# KERNEL ASSEMBLY
# ============================================================================

# Compile kernel assembly files
$(BUILD_DIR)/isr.o: $(KERNEL_DIR)/isr.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o: $(KERNEL_DIR)/switch.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# ============================================================================
# KERNEL C FILES (explicit rules to avoid ambiguity)
# ============================================================================

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

# ============================================================================
# LIBRARY FILES
# ============================================================================

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/lib/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

# ============================================================================
# LINKING
# ============================================================================

# Object files (explicit list)
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_ASM_OBJS = $(BUILD_DIR)/isr.o $(BUILD_DIR)/switch.o

# Link all object files into kernel ELF
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS) $(KERNEL_LIB_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# ============================================================================
# ISO CREATION
# ============================================================================

# Create bootable ISO image
$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.elf
	@echo "menuentry \"SYNAPSE SO\" {" > $(ISO_DIR)/boot/grub/grub.cfg
	@echo "    multiboot /boot/kernel.elf" >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo "    boot" >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo "}" >> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

# ============================================================================
# TESTING
# ============================================================================

# Run kernel in QEMU
run: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M

# Run kernel with debug output
debug: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M -d int,cpu_reset

# Run kernel in QEMU with GDB server
gdb: $(ISO_IMAGE)
	nohup qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M -s -S >/dev/null 2>&1 &
	@echo "QEMU started with GDB server on localhost:1234"
	@echo "Connect with: gdb build/kernel.elf"
	@echo "Then use: target remote :1234"

# ============================================================================
# CLEANUP
# ============================================================================

# Remove all build artifacts
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO_IMAGE)
	@echo "Clean complete."

# ============================================================================
# UTILITY TARGETS
# ============================================================================

# Clean and rebuild
rebuild: clean all

# Show kernel size information
size: $(KERNEL_BIN)
	@echo "Kernel size information:"
	size $(KERNEL_BIN)
	@echo ""
	@echo "Section sizes:"
	size -A $(KERNEL_BIN)

# Check if required tools are installed
check-tools:
	@echo "Checking for required build tools..."
	@which gcc > /dev/null && echo "✓ gcc" || echo "✗ gcc (NOT FOUND)"
	@which nasm > /dev/null && echo "✓ nasm" || echo "✗ nasm (NOT FOUND)"
	@which ld > /dev/null && echo "✓ ld" || echo "✗ ld (NOT FOUND)"
	@which grub-mkrescue > /dev/null && echo "✓ grub-mkrescue" || echo "✗ grub-mkrescue (NOT FOUND)"
	@which qemu-system-x86_64 > /dev/null && echo "✓ qemu-system-x86_64 (optional)" || echo "⚠ qemu-system-x86_64 (not installed, needed for 'run' target)"

# Show help
help:
	@echo "SYNAPSE SO Build System"
	@echo "======================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build kernel and ISO (default)"
	@echo "  run          - Run kernel in QEMU"
	@echo "  debug        - Run kernel in QEMU with debug output"
	@echo "  gdb          - Run kernel in QEMU with GDB server"
	@echo "  clean        - Remove build files"
	@echo "  rebuild      - Clean and rebuild"
	@echo "  size         - Show kernel size information"
	@echo "  check-tools  - Check if required tools are installed"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  Install tools: sudo apt-get install gcc-multilib nasm binutils grub-pc-bin xorriso qemu-system-x86"
	@echo "  Verify tools: make check-tools"

# ============================================================================
# PHONY TARGETS
# ============================================================================
.PHONY: all run debug gdb clean rebuild size check-tools help
