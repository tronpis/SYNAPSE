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

void timer_init(uint32_t frequency_hz) {
    __sync_add_and_fetch(&timer_ticks, 1);
}

uint32_t timer_get_ticks(void) {
    return (uint32_t)__sync_add_and_fetch(&timer_ticks, 0);
    return timer_ticks;
}
