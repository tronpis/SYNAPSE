/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SYNAPSE SO - Global Descriptor Table Header */

#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H


/* GDT initialization function */
void gdt_init(void);

/* Segment selectors */
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x1B
#define GDT_USER_DATA   0x23
/* Compile-time sanity checks: kernel selectors must have RPL 0, user selectors RPL 3 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert((GDT_KERNEL_CODE & 0x3) == 0, "GDT_KERNEL_CODE must have RPL 0");
_Static_assert((GDT_USER_CODE & 0x3) == 3, "GDT_USER_CODE must have RPL 3");
#endif
#endif /* KERNEL_GDT_H */
