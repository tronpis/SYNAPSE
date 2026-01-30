/* SYNAPSE SO - Serial Port Driver */
/* Licensed under GPLv3 */

#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1_BASE 0x3F8U

void serial_init(uint16_t base_port);
int serial_is_initialized(void);

void serial_write_char(char c);
void serial_write(const char* str);

#endif /* KERNEL_SERIAL_H */
