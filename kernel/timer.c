/* SYNAPSE SO - PIT Timer */
/* Licensed under GPLv3 */

#include <kernel/timer.h>
#include <kernel/io.h>
#include <kernel/vga.h>

#define PIT_FREQUENCY_HZ 1193180
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_COMMAND_MODE3 0x36

static volatile uint32_t timer_ticks;
static uint32_t timer_frequency __attribute__((unused));

void timer_init(uint32_t frequency_hz) {
    timer_ticks = 0;
    timer_frequency = frequency_hz;

    /* Calculate divisor: PIT_BASE / desired_frequency */
    if (frequency_hz == 0) {
        /* Avoid division by zero; clamp to 1 Hz minimum */
        frequency_hz = 1;
    }
    uint32_t divisor = PIT_FREQUENCY_HZ / frequency_hz;
    if (divisor > 65535) {
        divisor = 65535;
    }
    if (divisor < 1) {
        divisor = 1;
    }

    /* Send command to PIT: channel 0, lobyte/hibyte, mode 3 */
    outb(PIT_COMMAND_PORT, PIT_COMMAND_MODE3);

    /* Send divisor low byte */
    outb(PIT_CHANNEL0_PORT, divisor & 0xFF);

    /* Send divisor high byte */
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF);

    uint32_t actual_freq = PIT_FREQUENCY_HZ / divisor;
    vga_print("    Timer configured: ");
    vga_print_dec(actual_freq);
    vga_print(" Hz (requested: ");
    vga_print_dec(frequency_hz);
    vga_print(" Hz)\n");
}

void timer_increment_tick(void) {
    __sync_add_and_fetch(&timer_ticks, 1);
}

uint32_t timer_get_ticks(void) {
    return (uint32_t)__sync_add_and_fetch(&timer_ticks, 0);
}
