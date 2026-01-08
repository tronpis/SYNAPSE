/* SYNAPSE SO - Interrupt Descriptor Table Implementation */
/* Licensed under GPLv3 */

#include <kernel/idt.h>
#include <kernel/vga.h>

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

/* Interrupt handlers (assembly stubs) */
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
extern void isr_default(void); /* Default handler for unhandled interrupts */

/* Default interrupt handler stub (assembly) */
extern void isr_default(void);
extern void isr_common_stub(void);

/* Set an IDT gate */
static void idt_set_gate(unsigned char num, unsigned int base,
                         unsigned short sel, unsigned char flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

/* ISR handler called from assembly stub */

}

/* Initialize IDT */
void idt_init(void) {
    /* Setup IDT pointer */
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (unsigned int)&idt;




    /* Load IDT */
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
