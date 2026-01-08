/* SYNAPSE SO - Interrupt Descriptor Table Implementation */
/* Licensed under GPLv3 */

#include <kernel/idt.h>

/* IDT entry structure (for 32-bit) */
typedef struct {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_high;
} __attribute__((packed)) idt_entry_t;

/* IDT pointer structure */
typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_ptr_t;

/* IDT entries */
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

/* Interrupt handlers (minimal stubs) */
extern void isr0(void);  /* Divide by zero */
extern void isr1(void);  /* Debug */
extern void isr2(void);  /* Non-maskable interrupt */
extern void isr3(void);  /* Breakpoint */
extern void isr4(void);  /* Into detected overflow */
extern void isr5(void);  /* Out of bounds */
extern void isr6(void);  /* Invalid opcode */
extern void isr7(void);  /* No coprocessor */
extern void isr8(void);  /* Double fault */
extern void isr9(void);  /* Coprocessor segment overrun */
extern void isr10(void); /* Bad TSS */
extern void isr11(void); /* Segment not present */
extern void isr12(void); /* Stack fault */
extern void isr13(void); /* General protection fault */
extern void isr14(void); /* Page fault */
extern void isr15(void); /* Unknown interrupt */
extern void isr16(void); /* Coprocessor fault */
extern void isr17(void); /* Alignment check */
extern void isr18(void); /* Machine check */
extern void isr19(void); /* SIMD floating-point */
extern void isr20(void); /* Virtualization */
extern void isr21(void); /* Control protection */
extern void isr22(void); /* Reserved */
extern void isr23(void); /* Reserved */
extern void isr24(void); /* Reserved */
extern void isr25(void); /* Reserved */
extern void isr26(void); /* Reserved */
extern void isr27(void); /* Reserved */
extern void isr28(void); /* Reserved */
extern void isr29(void); /* Reserved */
extern void isr30(void); /* Reserved */
extern void isr31(void); /* Reserved */

/* Set an IDT gate */
static void idt_set_gate(unsigned char num, unsigned int base, 
                         unsigned short sel, unsigned char flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

/* Default interrupt handler */
static void default_isr_handler(void) {
    /* In a full implementation, this would log the interrupt */
    __asm__ __volatile__("iret");
}

/* ISR handler called from assembly stub */
void isr_handler(void) {
    /* Placeholder for ISR handling
     * In a full implementation, this would:
     * - Identify which interrupt occurred
     * - Log or handle the interrupt appropriately
     * - For page faults, handle memory management
     * - For system calls, switch to user space
     */
}

/* Initialize IDT */
void idt_init(void) {
    /* Setup IDT pointer */
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (unsigned int)&idt;

    /* Clear IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (unsigned int)default_isr_handler, 0x08, 0x8E);
    }

    /* Set up exception handlers */
    idt_set_gate(0, (unsigned int)isr0, 0x08, 0x8E);
    idt_set_gate(1, (unsigned int)isr1, 0x08, 0x8E);
    idt_set_gate(2, (unsigned int)isr2, 0x08, 0x8E);
    idt_set_gate(3, (unsigned int)isr3, 0x08, 0x8E);
    idt_set_gate(4, (unsigned int)isr4, 0x08, 0x8E);
    idt_set_gate(5, (unsigned int)isr5, 0x08, 0x8E);
    idt_set_gate(6, (unsigned int)isr6, 0x08, 0x8E);
    idt_set_gate(7, (unsigned int)isr7, 0x08, 0x8E);
    idt_set_gate(8, (unsigned int)isr8, 0x08, 0x8E);
    idt_set_gate(9, (unsigned int)isr9, 0x08, 0x8E);
    idt_set_gate(10, (unsigned int)isr10, 0x08, 0x8E);
    idt_set_gate(11, (unsigned int)isr11, 0x08, 0x8E);
    idt_set_gate(12, (unsigned int)isr12, 0x08, 0x8E);
    idt_set_gate(13, (unsigned int)isr13, 0x08, 0x8E);
    idt_set_gate(14, (unsigned int)isr14, 0x08, 0x8E);
    idt_set_gate(15, (unsigned int)isr15, 0x08, 0x8E);
    idt_set_gate(16, (unsigned int)isr16, 0x08, 0x8E);
    idt_set_gate(17, (unsigned int)isr17, 0x08, 0x8E);
    idt_set_gate(18, (unsigned int)isr18, 0x08, 0x8E);
    idt_set_gate(19, (unsigned int)isr19, 0x08, 0x8E);
    idt_set_gate(20, (unsigned int)isr20, 0x08, 0x8E);
    idt_set_gate(21, (unsigned int)isr21, 0x08, 0x8E);
    idt_set_gate(22, (unsigned int)isr22, 0x08, 0x8E);
    idt_set_gate(23, (unsigned int)isr23, 0x08, 0x8E);
    idt_set_gate(24, (unsigned int)isr24, 0x08, 0x8E);
    idt_set_gate(25, (unsigned int)isr25, 0x08, 0x8E);
    idt_set_gate(26, (unsigned int)isr26, 0x08, 0x8E);
    idt_set_gate(27, (unsigned int)isr27, 0x08, 0x8E);
    idt_set_gate(28, (unsigned int)isr28, 0x08, 0x8E);
    idt_set_gate(29, (unsigned int)isr29, 0x08, 0x8E);
    idt_set_gate(30, (unsigned int)isr30, 0x08, 0x8E);
    idt_set_gate(31, (unsigned int)isr31, 0x08, 0x8E);

    /* Load IDT */
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
