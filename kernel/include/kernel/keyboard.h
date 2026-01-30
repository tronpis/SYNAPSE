/* SYNAPSE SO - PS/2 Keyboard Driver */
/* Licensed under GPLv3 */

#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
void keyboard_irq_handler(void);

int keyboard_has_char(void);
char keyboard_get_char(void);

#endif /* KERNEL_KEYBOARD_H */
