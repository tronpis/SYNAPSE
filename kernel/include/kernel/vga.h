/* SYNAPSE SO - VGA Text Mode Driver Header */
/* Licensed under GPLv3 */

#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

#ifdef __cplusplus
extern "C" {
#endif

/* VGA dimensions */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA colors */
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

/* VGA functions */
void vga_clear_screen(void);
void vga_set_color(unsigned char fg, unsigned char bg);
void vga_put_char(char c);
void vga_print(const char* str);
void vga_print_dec(unsigned int num);
void vga_print_hex(unsigned int num);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_VGA_H */
