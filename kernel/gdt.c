/* SYNAPSE SO - Global Descriptor Table Implementation */
/* Licensed under GPLv3 */

#include <kernel/gdt.h>

/* GDT entry structure */
typedef struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer structure */
typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) gdt_ptr_t;

/* GDT entries */
static gdt_entry_t gdt[5];
static gdt_ptr_t gdt_ptr;

/* TSS structure (not used in minimal version) */
typedef struct {
    unsigned int prev_tss;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed)) tss_entry_t;

static tss_entry_t tss;

/* Function to set a GDT entry */
static void gdt_set_entry(int num, unsigned int base, unsigned int limit, 
                          unsigned char access, unsigned char gran) {
    /* Set base address */
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Set limits */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    /* Set granularity and flags */
    gdt[num].granularity |= gran & 0xF0;

    /* Set access byte */
    gdt[num].access = access;
}

/* Initialize GDT */
void gdt_init(void) {
    /* Setup GDT pointer */
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base = (unsigned int)&gdt;

    /* Clear GDT */
    for (int i = 0; i < 5; i++) {
        gdt_set_entry(i, 0, 0, 0, 0);
    }

    /* Set up GDT entries:
     * 0: Null segment
     * 1: Kernel Code segment (base=0, limit=4GB, type=code, ring=0)
     * 2: Kernel Data segment (base=0, limit=4GB, type=data, ring=0)
     * 3: User Code segment (base=0, limit=4GB, type=code, ring=3)
     * 4: User Data segment (base=0, limit=4GB, type=data, ring=3)
     */

    /* Kernel Code Segment */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel Data Segment */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* User Code Segment */
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* User Data Segment */
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Load GDT */
    __asm__ __volatile__("lgdt %0" : : "m"(gdt_ptr));

    /* Reload segment registers */
    unsigned int temp;
    __asm__ __volatile__(
        "movw $0x10, %ax\n"
        "movw %ax, %ds\n"
        "movw %ax, %es\n"
        "movw %ax, %fs\n"
        "movw %ax, %gs\n"
        "movw %ax, %ss\n"
        : : : "ax"
    );
}
