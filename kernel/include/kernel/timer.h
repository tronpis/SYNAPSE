/* SYNAPSE SO - PIT Timer */
/* Licensed under GPLv3 */

#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <stdint.h>

/* Timer initialization */
void timer_init(uint32_t frequency_hz);

/* Timer tick management */
void timer_increment_tick(void);
uint32_t timer_get_ticks(void);

/* Timer frequency */
uint32_t timer_get_frequency(void);

/* Sleep support */
void timer_sleep(uint32_t ticks);

/* Uptime in seconds */
uint32_t timer_get_uptime_seconds(void);

/* Uptime in milliseconds (approximate) */
uint32_t timer_get_uptime_ms(void);

#endif /* KERNEL_TIMER_H */
