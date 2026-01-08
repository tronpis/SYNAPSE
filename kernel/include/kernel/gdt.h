/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SYNAPSE SO - Global Descriptor Table Header */

#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

/* Segment selector constants */
#define KERNEL_CS 0x08  /* Kernel code segment selector (index 1 << 3) */
#define KERNEL_DS 0x10  /* Kernel data segment selector (index 2 << 3) */
#define USER_CS   0x1B  /* User code segment selector (index 3 << 3) | RPL=3 */
#define USER_DS   0x20  /* User data segment selector (index 4 << 3) */

/* GDT initialization function */
void gdt_init(void);

#endif /* KERNEL_GDT_H */
