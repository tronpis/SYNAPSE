/* SYNAPSE SO - Interrupt Descriptor Table Header */
/* Licensed under GPLv3 */

#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#ifdef __cplusplus
extern "C" {
#endif

/* IDT initialization function */
void idt_init(void);

/* ISR handler function (called from assembly) */
void isr_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_IDT_H */
