/* SYNAPSE SO - PS/2 Keyboard Driver */
/* Licensed under GPLv3 */

#include <kernel/keyboard.h>
#include <kernel/io.h>

#define KBD_DATA_PORT   0x60U
#define KBD_STATUS_PORT 0x64U

#define KBD_STATUS_OUTPUT_FULL 0x01U

#define KBD_SC_LSHIFT_PRESS  0x2AU
#define KBD_SC_RSHIFT_PRESS  0x36U
#define KBD_SC_LSHIFT_RELEASE 0xAAU
#define KBD_SC_RSHIFT_RELEASE 0xB6U

#define KBD_BUF_SIZE 128U

static volatile char kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head = 0U;
static volatile uint32_t kbd_tail = 0U;
static volatile uint8_t shift_down = 0U;

static const char keymap[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0,
    ' ', 0
};

static const char keymap_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0,
    ' ', 0
};

static void kbd_push_char(char c) {
    uint32_t next = (kbd_head + 1U) % KBD_BUF_SIZE;
    if (next == kbd_tail) {
        return;
    }

    kbd_buf[kbd_head] = c;
    kbd_head = next;
}

int keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}

char keyboard_get_char(void) {
    if (kbd_head == kbd_tail) {
        return 0;
    }

    char c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1U) % KBD_BUF_SIZE;
    return c;
}

static void kbd_flush_output_buffer(void) {
    for (uint32_t i = 0; i < 256U; i++) {
        uint8_t status = inb(KBD_STATUS_PORT);
        if ((status & KBD_STATUS_OUTPUT_FULL) == 0U) {
            break;
        }
        (void)inb(KBD_DATA_PORT);
    }
}

void keyboard_init(void) {
    kbd_head = 0U;
    kbd_tail = 0U;
    shift_down = 0U;

    kbd_flush_output_buffer();
}

void keyboard_irq_handler(void) {
    uint8_t scancode = inb(KBD_DATA_PORT);

    if (scancode == KBD_SC_LSHIFT_PRESS || scancode == KBD_SC_RSHIFT_PRESS) {
        shift_down = 1U;
        return;
    }

    if (scancode == KBD_SC_LSHIFT_RELEASE || scancode == KBD_SC_RSHIFT_RELEASE) {
        shift_down = 0U;
        return;
    }

    if ((scancode & 0x80U) != 0U) {
        return;
    }

    char c = shift_down ? keymap_shift[scancode] : keymap[scancode];
    if (c == 0) {
        return;
    }

    kbd_push_char(c);
}
