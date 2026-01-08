/* SYNAPSE SO - Kernel Linker Script */
/* Licensed under GPLv3 */

ENTRY(_start)

SECTIONS
{
    /* The kernel starts at 1MB */
    . = 1M;

    /* Multiboot header and boot code */
    .text : ALIGN(4K)
    {
        KEEP(*(.multiboot))
        *(.text*)
    }

    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K)
    {
        *(.data*)
    }

    /* Read-write data (uninitialized) and stack */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Discard sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
