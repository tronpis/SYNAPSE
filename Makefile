# SYNAPSE SO - Makefile
# Licensed under GPLv3

# Compiler and tools
CC = gcc
AS = nasm
LD = ld
GRUB_MKRESCUE = grub-mkrescue

# Compiler flags
CFLAGS = -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pie -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T boot/linker.ld
ASFLAGS = -f elf32

# Directories
KERNEL_DIR = kernel
BOOT_DIR = boot
BUILD_DIR = build
ISO_DIR = isodir

# Source files
BOOT_ASM = $(BOOT_DIR)/boot.asm
KERNEL_ASM = $(KERNEL_DIR)/isr.asm
KERNEL_C = $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/vga.c $(KERNEL_DIR)/gdt.c $(KERNEL_DIR)/idt.c
KERNEL_LIB = $(KERNEL_DIR)/lib/string.c

# Object files
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_ASM_OBJ = $(BUILD_DIR)/isr.o
KERNEL_C_OBJ = $(KERNEL_C:.c=.o)
KERNEL_C_OBJ := $(addprefix $(BUILD_DIR)/, $(notdir $(KERNEL_C_OBJ)))
KERNEL_LIB_OBJ = $(KERNEL_LIB:.c=.o)
KERNEL_LIB_OBJ := $(addprefix $(BUILD_DIR)/, $(notdir $(KERNEL_LIB_OBJ)))

# Output files
KERNEL_BIN = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = synapse.iso

# Default target
all: $(ISO_IMAGE)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile boot assembly
$(BOOT_OBJ): $(BOOT_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile kernel assembly
$(KERNEL_ASM_OBJ): $(KERNEL_ASM) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile kernel C files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

# Compile library files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/lib/%.c | $(BUILD_DIR)
    @mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

# Link kernel
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ) $(KERNEL_LIB_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Create ISO image
$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.elf
	@echo "menuentry \"SYNAPSE SO\" {" > $(ISO_DIR)/boot/grub/grub.cfg
	@echo "    multiboot /boot/kernel.elf" >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo "    boot" >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo "}" >> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

# Run kernel in QEMU
run: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M

# Run kernel with debug
debug: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M -d int,cpu_reset

# Clean build files
clean:
	@rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO_IMAGE)

# Rebuild
rebuild: clean all

# Show size
size: $(KERNEL_BIN)
	size $(KERNEL_BIN)

# Help
help:
	@echo "SYNAPSE SO Build System"
	@echo "======================="
	@echo "Targets:"
	@echo "  all      - Build kernel and ISO (default)"
	@echo "  run      - Run kernel in QEMU"
	@echo "  debug    - Run kernel in QEMU with debug"
	@echo "  clean    - Remove build files"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  size     - Show kernel size information"
	@echo "  help     - Show this help message"

.PHONY: all run debug clean rebuild size help
