/* SYNAPSE SO - Interrupt Descriptor Table Header */
/* Licensed under GPLv3 */

#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Register state structure */
typedef struct {
    unsigned int gs, fs, es, ds;      /* Segment registers */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; /* Pushed by pusha */
    unsigned int int_no, err_code;    /* Interrupt number and error code */
    unsigned int eip, cs, eflags;     /* Pushed by CPU */
} registers_t;

/* IDT initialization function */
void idt_init(void);

/* ISR handler function (called from assembly) */
void isr_handler(registers_t *regs);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_IDT_H */
