/* SYNAPSE SO - Serial Port Driver */
/* Licensed under GPLv3 */

#include <kernel/serial.h>
#include <kernel/io.h>

#define SERIAL_REG_DATA 0U
#define SERIAL_REG_IER  1U
#define SERIAL_REG_LCR  3U
#define SERIAL_REG_FCR  2U
#define SERIAL_REG_MCR  4U
#define SERIAL_REG_LSR  5U

#define SERIAL_LSR_THRE 0x20U

static uint16_t serial_base = 0U;
static int serial_ready = 0;

static int serial_tx_ready(void) {
    if (serial_ready == 0) {
        return 0;
    }

    return (inb((unsigned short)(serial_base + SERIAL_REG_LSR)) &
            SERIAL_LSR_THRE) != 0U;
}

void serial_init(uint16_t base_port) {
    serial_base = base_port;

    /* Disable interrupts */
    outb((unsigned short)(serial_base + SERIAL_REG_IER), 0x00U);

    /* Enable DLAB */
    outb((unsigned short)(serial_base + SERIAL_REG_LCR), 0x80U);

    /* Set divisor to 1 (115200 baud) */
    outb((unsigned short)(serial_base + SERIAL_REG_DATA), 0x01U);
    outb((unsigned short)(serial_base + SERIAL_REG_IER), 0x00U);

    /* 8 bits, no parity, one stop bit (disable DLAB) */
    outb((unsigned short)(serial_base + SERIAL_REG_LCR), 0x03U);

    /* Enable FIFO, clear, 14-byte threshold */
    outb((unsigned short)(serial_base + SERIAL_REG_FCR), 0xC7U);

    /* IRQs disabled, RTS/DSR set */
    outb((unsigned short)(serial_base + SERIAL_REG_MCR), 0x03U);

    serial_ready = 1;
}

int serial_is_initialized(void) {
    return serial_ready != 0;
}

void serial_write_char(char c) {
    if (serial_ready == 0) {
        return;
    }

    if (c == '\n') {
        serial_write_char('\r');
    }

    if (c == '\b') {
        while (serial_tx_ready() == 0) {
            __asm__ __volatile__("pause");
        }
        outb((unsigned short)(serial_base + SERIAL_REG_DATA), (unsigned char)'\b');
        while (serial_tx_ready() == 0) {
            __asm__ __volatile__("pause");
        }
        outb((unsigned short)(serial_base + SERIAL_REG_DATA), (unsigned char)' ');
        while (serial_tx_ready() == 0) {
            __asm__ __volatile__("pause");
        }
        outb((unsigned short)(serial_base + SERIAL_REG_DATA), (unsigned char)'\b');
        return;
    }

    while (serial_tx_ready() == 0) {
        __asm__ __volatile__("pause");
    }

    outb((unsigned short)(serial_base + SERIAL_REG_DATA), (unsigned char)c);
}

void serial_write(const char* str) {
    if (serial_ready == 0 || str == 0) {
        return;
    }

    for (uint32_t i = 0; str[i] != '\0'; i++) {
        serial_write_char(str[i]);
    }
}
