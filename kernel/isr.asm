; SYNAPSE SO - Interrupt Service Routines
; Licensed under GPLv3

section .text

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

; Default ISR for unhandled interrupts
global isr_default
isr_default:
    cli
    push byte 0      ; dummy error code
    push dword 255   ; reserved ISR number for unassigned
    jmp isr_common_stub

; Common ISR handler - called by all ISRs
; Stack layout on entry:
;   [esp+20] Error code (or dummy 0)
;   [esp+16] ISR number
;   [esp+12] EIP
;   [esp+8]  CS
;   [esp+4]  EFLAGS
;   [esp]    Error code (for exceptions that push it)
; Note: CPU state is NOT yet saved
global isr_common_stub
extern isr_handler

isr_common_stub:
    ; Save general-purpose registers
    pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment (0x10 = index 2 << 3)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler
    push esp                 ; Push pointer to registers_t struct
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
