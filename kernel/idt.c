/* SYNAPSE SO - Interrupt Descriptor Table Implementation */
/* Licensed under GPLv3 */

#include <kernel/idt.h>
#include <kernel/vga.h>
#include <kernel/io.h>
#include <kernel/gdt.h>
#include <kernel/vmm.h>
#include <kernel/scheduler.h>
#include <kernel/timer.h>

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

/* IRQ stubs */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

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
registers_t* isr_handler(registers_t *regs) {
    if (regs == 0) {
        return 0;
    }

    /* Identify which interrupt occurred */
    if (regs->int_no < 32) {
        /* Exception handling */
        switch (regs->int_no) {
            case 14: /* Page fault */
                vmm_page_fault_handler(regs->err_code);
                break;

            default:
                /* Prevent further interrupts while we print halt message */
                __asm__ __volatile__("cli");
                vga_print("\n[EXCEPTION] ");
                vga_print_dec(regs->int_no);
                vga_print(" - Error Code: ");
                vga_print_hex(regs->err_code);
                vga_print("\nKernel Halted.\n");
                while (1) {
                    __asm__ __volatile__("hlt");
                }
                break;
        }

        return regs;
    }

    if (regs->int_no >= 32 && regs->int_no <= 47) {
        registers_t* new_regs = regs;

        /* Send EOI early before scheduler_tick (which may context switch).
           Safety: Scheduler must not assume IRQ ownership after EOI.
           - EOI acknowledges IRQ to PIC, allowing nested IRQs if re-enabled
           - Scheduler only selects next process and returns registers_t*
           - Assembly stub (isr_common_stub) uses returned pointer to restore context
           - iret restores EFLAGS which re-enables interrupts
           This is safe because no code assumes IRQ state after EOI. */
        if (regs->int_no >= 40) {
            /* Slave PIC */
            outb(0xA0, 0x20);
        }
        /* Master PIC */
        outb(0x20, 0x20);

        /* IRQ0: PIT timer */
        if (regs->int_no == 32) {
            timer_increment_tick();
            new_regs = scheduler_tick(regs);
            if (new_regs == 0) {
                new_regs = regs;
            }
        }

        return new_regs;
    }

    return regs;
}

/* Initialize IDT */
void idt_init(void) {
    /* Setup IDT pointer */
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (unsigned int)&idt;

    /* Clear IDT - set all entries to default stub */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (unsigned int)isr_default, GDT_KERNEL_CODE, 0x8E);
    }

    /* Set up exception handlers with specific ISRs */
    idt_set_gate(0, (unsigned int)isr0, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(1, (unsigned int)isr1, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(2, (unsigned int)isr2, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(3, (unsigned int)isr3, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(4, (unsigned int)isr4, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(5, (unsigned int)isr5, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(6, (unsigned int)isr6, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(7, (unsigned int)isr7, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(8, (unsigned int)isr8, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(9, (unsigned int)isr9, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(10, (unsigned int)isr10, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(11, (unsigned int)isr11, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(12, (unsigned int)isr12, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(13, (unsigned int)isr13, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(14, (unsigned int)isr14, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(15, (unsigned int)isr15, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(16, (unsigned int)isr16, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(17, (unsigned int)isr17, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(18, (unsigned int)isr18, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(19, (unsigned int)isr19, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(20, (unsigned int)isr20, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(21, (unsigned int)isr21, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(22, (unsigned int)isr22, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(23, (unsigned int)isr23, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(24, (unsigned int)isr24, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(25, (unsigned int)isr25, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(26, (unsigned int)isr26, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(27, (unsigned int)isr27, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(28, (unsigned int)isr28, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(29, (unsigned int)isr29, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(30, (unsigned int)isr30, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(31, (unsigned int)isr31, GDT_KERNEL_CODE, 0x8E);

    /* Remap PIC */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    /* Set up IRQ handlers */
    idt_set_gate(32, (unsigned int)irq0, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(33, (unsigned int)irq1, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(34, (unsigned int)irq2, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(35, (unsigned int)irq3, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(36, (unsigned int)irq4, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(37, (unsigned int)irq5, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(38, (unsigned int)irq6, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(39, (unsigned int)irq7, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(40, (unsigned int)irq8, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(41, (unsigned int)irq9, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(42, (unsigned int)irq10, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(43, (unsigned int)irq11, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(44, (unsigned int)irq12, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(45, (unsigned int)irq13, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(46, (unsigned int)irq14, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(47, (unsigned int)irq15, GDT_KERNEL_CODE, 0x8E);

    /* Load IDT */
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
