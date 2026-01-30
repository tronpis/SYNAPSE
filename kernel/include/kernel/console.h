/* SYNAPSE SO - Console */
/* Licensed under GPLv3 */

#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include <stdint.h>

void console_init(void);

char console_get_char(void);
uint32_t console_read_line(char* buf, uint32_t max_len);

#endif /* KERNEL_CONSOLE_H */
