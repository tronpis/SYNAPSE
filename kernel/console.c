/* SYNAPSE SO - Console */
/* Licensed under GPLv3 */

#include <kernel/console.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>

void console_init(void) {
    /* Currently no explicit initialization is required. */
}

char console_get_char(void) {
    while (keyboard_has_char() == 0) {
        __asm__ __volatile__("hlt");
    }

    return keyboard_get_char();
}

uint32_t console_read_line(char* buf, uint32_t max_len) {
    if (buf == 0 || max_len == 0U) {
        return 0;
    }

    uint32_t len = 0U;

    while (1) {
        char c = console_get_char();

        if (c == '\r') {
            c = '\n';
        }

        if (c == '\n') {
            vga_put_char('\n');
            buf[len] = '\0';
            return len;
        }

        if (c == '\b') {
            if (len > 0U) {
                len--;
                vga_put_char('\b');
            }
            continue;
        }

        if (c < ' ') {
            continue;
        }

        if ((len + 1U) >= max_len) {
            continue;
        }

        buf[len] = c;
        len++;
        vga_put_char(c);
    }
}
