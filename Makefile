# SYNAPSE SO - Makefile
# Licensed under GPLv3


$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR)/include -c $< -o $@

$(BUILD_DIR)/vga.o: $(KERNEL_DIR)/vga.c | $(BUILD_DIR)

