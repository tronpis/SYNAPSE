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
static uint32_t timer_frequency;

void timer_init(uint32_t frequency_hz) {
    timer_ticks = 0;
    timer_frequency = frequency_hz;

    /* Calculate divisor: PIT_BASE / desired_frequency */
    if (frequency_hz == 0) {
        /* Avoid division by zero; clamp to 1 Hz minimum */
        frequency_hz = 1;
        timer_frequency = 1;
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

    /* Update actual frequency based on divisor */
    timer_frequency = PIT_FREQUENCY_HZ / divisor;

    vga_print("    Timer configured: ");
    vga_print_dec(timer_frequency);
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

uint32_t timer_get_frequency(void) {
    return timer_frequency;
}

void timer_sleep(uint32_t ticks) {
    uint32_t start = timer_get_ticks();
    uint32_t target = start + ticks;
    
    /* Handle overflow */
    if (target < start) {
        /* Wait for overflow */
        while (timer_get_ticks() >= start) {
            __asm__ __volatile__("hlt");
        }
    }
    
    /* Wait for target ticks */
    while (timer_get_ticks() < target) {
        __asm__ __volatile__("hlt");
    }
}

uint32_t timer_get_uptime_seconds(void) {
    if (timer_frequency == 0) {
        return 0;
    }
    return timer_get_ticks() / timer_frequency;
}

uint32_t timer_get_uptime_ms(void) {
    if (timer_frequency == 0) {
        return 0;
    }
    /* Calculate milliseconds: (ticks * 1000) / frequency */
    /* Avoid overflow by dividing first if ticks is large */
    uint32_t ticks = timer_get_ticks();
    if (ticks > 4294967) {
        /* Large tick count - divide first */
        return (ticks / timer_frequency) * 1000 + 
               ((ticks % timer_frequency) * 1000) / timer_frequency;
    }
    return (ticks * 1000) / timer_frequency;
}
