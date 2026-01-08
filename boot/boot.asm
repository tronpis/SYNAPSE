; SYNAPSE SO - Bootloader Entry Point
; Minimal boot setup for i386 (32-bit) kernel
; Licensed under GPLv3

section .multiboot
align 8
mb_header_start:
    dd 0xE85250D6                 ; magic number
    dd 0                          ; architecture 0 (i386)
    dd mb_header_end - mb_header_start ; header length
    dd -(0xE85250D6 + 0 + (mb_header_end - mb_header_start)) ; checksum
    dd 0                          ; end tag
mb_header_end:

section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top

    ; Save Multiboot info pointer
    push ebx

    ; Call kernel main
    call kernel_main

    ; Infinite loop if kernel returns
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KiB stack
stack_top:
