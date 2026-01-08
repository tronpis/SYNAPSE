; SYNAPSE SO - Interrupt Service Routines
; Licensed under GPLv3

section .text

; Segment selector constants (must match kernel/include/kernel/gdt.h)
%define GDT_KERNEL_CODE 0x08
%define GDT_KERNEL_DATA 0x10

; Macro for ISR without error code
; These push a dummy error code to keep stack uniform
%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    cli
    push byte 0      ; Push dummy error code for consistency
    push byte %1     ; Push ISR number
    jmp isr_common_stub
%endmacro

; Macro for ISR with error code
; These don't push dummy error code - CPU pushes it
%macro ISR_ERRCODE 1
  global isr%1
  isr%1:
    cli
    push byte %1     ; Push ISR number (error code already pushed by CPU)
    jmp isr_common_stub
%endmacro

; Define all ISRs
; Exceptions 0-31
ISR_NOERRCODE 0   ; Divide by zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; Non-maskable interrupt
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Into detected overflow
ISR_NOERRCODE 5   ; Out of bounds
ISR_NOERRCODE 6   ; Invalid opcode
ISR_NOERRCODE 7   ; No coprocessor
ISR_ERRCODE   8   ; Double fault
ISR_NOERRCODE 9   ; Coprocessor segment overrun
ISR_ERRCODE   10  ; Bad TSS
ISR_ERRCODE   11  ; Segment not present
ISR_ERRCODE   12  ; Stack fault
ISR_ERRCODE   13  ; General protection fault
ISR_ERRCODE   14  ; Page fault
ISR_NOERRCODE 15  ; Unknown interrupt
ISR_NOERRCODE 16  ; Coprocessor fault
ISR_ERRCODE   17  ; Alignment check
ISR_NOERRCODE 18  ; Machine check
ISR_NOERRCODE 19  ; SIMD floating-point
ISR_NOERRCODE 20  ; Virtualization
ISR_ERRCODE   21  ; Control protection
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Reserved
ISR_NOERRCODE 29  ; Reserved
ISR_NOERRCODE 30  ; Reserved
ISR_NOERRCODE 31  ; Reserved

; IRQs
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push byte 0
    push byte %2
    jmp isr_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; Default ISR for unhandled interrupts
global isr_default
isr_default:
    cli
    push byte 0      ; dummy error code
    push dword 255   ; reserved ISR number for unassigned
    jmp isr_common_stub

extern isr_handler

isr_common_stub:
    ; Save general-purpose registers
    pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    ; Save segment registers
    push ds
    push es
    push fs
    push gs


    mov ax, GDT_KERNEL_DATA        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler
    mov eax, esp
    push eax                 ; Push pointer to registers_t struct
    call isr_handler
    add esp, 4               ; Clean up argument from stack


    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general-purpose registers
    popa

    ; Clean up error code and ISR number from stack
    ; These were pushed by ISR macros before calling common stub
    add esp, 8

    ; Return from interrupt
    iret
section .note.GNU-stack noalloc noexec nowrite progbits
