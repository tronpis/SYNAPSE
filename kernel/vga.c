/* SYNAPSE SO - VGA Text Mode Driver */
/* Licensed under GPLv3 */

#include <kernel/vga.h>

/* VGA memory buffer */
volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;

/* Current cursor position */
static int cursor_x = 0;
static int cursor_y = 0;

/* Current color scheme */
static unsigned char current_color = VGA_COLOR_LIGHT_GREY;

/* Clear the screen */
void vga_clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (unsigned short)' ' | (current_color << 8);
    }
    cursor_x = 0;
    cursor_y = 0;
}

/* Set text color */
void vga_set_color(unsigned char fg, unsigned char bg) {
    current_color = (bg << 4) | (fg & 0x0F);
}

/* Scroll screen up one line */
static void vga_scroll(void) {
    /* Move all lines up */
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    /* Clear last line */
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = (unsigned short)' ' | (current_color << 8);
    }

    cursor_y = VGA_HEIGHT - 1;
}

/* Put a character at current position */
void vga_put_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c >= ' ') {
        /* Regular character */
        int offset = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[offset] = (unsigned short)c | (current_color << 8);
        cursor_x++;
    }

    /* Handle line wrapping and scrolling */
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
}

/* Print a null-terminated string */
void vga_print(const char* str) {
    while (*str) {
        vga_put_char(*str++);
    }
}

/* Print a decimal number */
void vga_print_dec(unsigned int num) {
    if (num == 0) {
        vga_put_char('0');
        return;
    }

    char buffer[11];
    int i = 0;

    /* Convert number to string (reversed) */
    while (num > 0 && i < 11) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    /* Print in reverse order */
    while (i > 0) {
        vga_put_char(buffer[--i]);
    }
}

/* Print a hexadecimal number */
void vga_print_hex(unsigned int num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];

    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[num & 0xF];
        num >>= 4;
    }

    buffer[10] = '\0';
    vga_print(buffer);
}
