/* SYNAPSE SO - User Mode Transition */
/* Licensed under GPLv3 */

#ifndef KERNEL_USERMODE_H
#define KERNEL_USERMODE_H

#include <stdint.h>

/* Enter user mode and jump to given address
 * This function does not return - it switches to user mode
 * and starts executing at entry_point with user stack
 */
void enter_usermode(uint32_t entry_point, uint32_t user_stack) __attribute__((noreturn));

/* Create a simple user process for testing
 * Returns process ID or 0 on failure
 */
uint32_t create_user_test_process(void);

#endif /* KERNEL_USERMODE_H */
