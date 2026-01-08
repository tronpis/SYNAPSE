# SYNAPSE SO - Referencia Técnica

## Referencia de Implementación del Kernel

Este documento proporciona detalles técnicos profundos sobre la implementación del kernel de SYNAPSE SO.

---

## Tabla de Contenidos

1. [Especificaciones Multiboot](#especificaciones-multiboot)
2. [GDT - Detalles de Implementación](#gdt---detalles-de-implementación)
3. [IDT - Detalles de Implementación](#idt---detalles-de-implementación)
4. [VGA - Programación del Hardware](#vga---programación-del-hardware)
5. [Convenciones de Llamada](#convenciones-de-llamada)
6. [Formatos de Archivos](#formatos-de-archivos)
7. [Constantes y Definiciones](#constantes-y-definiciones)

---

## Especificaciones Multiboot

### Header de Multiboot

El kernel cumple con la especificación Multiboot v1. Implementada en `boot/boot.asm`:

```assembly
section .multiboot
align 8
mb_header_start:
    dd 0xE85250D6                 ; magic number
    dd 0                          ; architecture 0 (i386)
    dd mb_header_end - mb_header_start ; header length
    dd -(0xE85250D6 + 0 + (mb_header_end - mb_header_start)) ; checksum
    dd 0                          ; end tag
mb_header_end:
```

**Detalles del Header**:

| Campo | Offset | Tamaño | Valor |
|-------|--------|--------|-------|
| Magic | 0 | 4 | 0xE85250D6 |
| Arch | 4 | 4 | 0 (i386) |
| Length | 8 | 4 | Header length |
| Checksum | 12 | 4 | Negated sum |

### Información de Multiboot

Puntero pasado en `EBX` (x86) o `RBX` (x86_64):

```c
typedef struct {
    unsigned int flags;           // Flags disponibles
    unsigned int mem_lower;       // Memoria baja (KB)
    unsigned int mem_upper;       // Memoria alta (KB)
    unsigned int boot_device;     // Dispositivo de boot
    unsigned int cmdline;         // Línea de comandos
    unsigned int mods_count;      // Número de módulos
    unsigned int mods_addr;        // Dirección de módulos
    // ... más campos no usados en versión mínima
} __attribute__((packed)) multiboot_info_t;
```

**Magic Number**: 0x2BADB002 (debe ser validado en kernel_main)

---

## GDT - Detalles de Implementación

### Estructura de Entrada GDT

Cada entrada GDT tiene 8 bytes (64 bits):

```
Bits:
[63:56] Base[31:24]
[55:52] Granularity & Limit[19:16]
[51:48] Access byte
[47:40] Base[23:16]
[39:32] Base[15:0]
[31:16] Limit[15:0]
```

### Campos Detallados

#### Byte de Acceso (bits 40:47)

```
Bit 7: Present (P) - Debe ser 1
Bit 6-5: DPL (Descriptor Privilege Level)
  00 = Ring 0 (Kernel)
  11 = Ring 3 (User)
Bit 4: S (System) - 0 para system segments, 1 para code/data
Bit 3-0: Type
  Para segmentos de código:
    1000 = Execute-Only
    1010 = Execute/Read
  Para segmentos de datos:
    0000 = Read-Only
    0010 = Read/Write
```

#### Byte de Granularidad (bits 52:55)

```
Bit 7: Granularity (G)
  0 = 1-byte granularity
  1 = 4KB granularity
Bit 6: Size (D)
  0 = 16-bit protected mode
  1 = 32-bit protected mode
Bit 5: Long Mode (L) - 0 para 32-bit
Bit 4: AVL - Disponible para software
```

### Entradas del Kernel SYNAPSE SO

| Índice | Nombre | Base | Límite | DPL | Tipo | Descripción |
|--------|-------|------|--------|-----|------|-------------|
| 0 | Null | 0 | 0 | 00 | - | Requerido por x86 |
| 1 | Kernel Code | 0 | 4GB | 00 | 0x9A | Execute/Read, Ring 0 |
| 2 | Kernel Data | 0 | 4GB | 00 | 0x92 | Read/Write, Ring 0 |
| 3 | User Code | 0 | 4GB | 11 | 0xFA | Execute/Read, Ring 3 |
| 4 | User Data | 0 | 4GB | 11 | 0xF2 | Read/Write, Ring 3 |

**Segment Selectors**:

```c
#define KERNEL_CS 0x08  // Índice 1 << 3
#define KERNEL_DS 0x10  // Índice 2 << 3
#define USER_CS   0x18  // Índice 3 << 3
#define USER_DS   0x20  // Índice 4 << 3
```

### Carga del GDT

```assembly
; Cargar GDTR
lgdt [gdt_ptr]

; Recargar segment registers
mov ax, KERNEL_DS
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
```

**Estructura GDTR**:
```c
typedef struct {
    unsigned short limit;  // Tamaño de GDT - 1
    unsigned int base;    // Dirección de GDT
} __attribute__((packed)) gdt_ptr_t;
```

---

## IDT - Detalles de Implementación

### Estructura de Entrada IDT (32-bit)

Cada entrada IDT tiene 8 bytes (64 bits):

```
Bits:
[63:48] Offset[31:16]
[47:40] Type/Attributes
[39:32] Reserved (0)
[31:16] Selector (segment selector)
[15:0]  Offset[15:0]
```

### Byte de Tipo/Atributos (bits 40:47)

```
Bit 7: Present (P) - Debe ser 1
Bit 6-5: DPL (Descriptor Privilege Level)
Bit 4: Storage Segment (S) - 0 para interrupt gate
Bit 3-0: Type
  1110 = 32-bit Interrupt Gate
  1111 = 32-bit Trap Gate
```

**Interrupt Gate vs Trap Gate**:
- Interrupt Gate: Deshabilita IRQs (CLI automático)
- Trap Gate: Mantiene estado de IRQs

### Vectores de Excepción x86

| Vector | Nombre | Tipo | Error Code |
|--------|-------|------|------------|
| 0 | Divide Error | Fault | No |
| 1 | Debug | Fault/Trap | No |
| 2 | NMI | Interrupt | No |
| 3 | Breakpoint | Trap | No |
| 4 | Into Detected Overflow | Trap | No |
| 5 | Out of Bounds | Fault | No |
| 6 | Invalid Opcode | Fault | No |
| 7 | Device Not Available | Fault | No |
| 8 | Double Fault | Abort | Sí |
| 9 | Coprocessor Segment Overrun | Fault | No |
| 10 | Invalid TSS | Fault | Sí |
| 11 | Segment Not Present | Fault | Sí |
| 12 | Stack-Segment Fault | Fault | Sí |
| 13 | General Protection | Fault | Sí |
| 14 | Page Fault | Fault | Sí |
| 15 | (Reserved) | - | - |
| 16 | x87 FPU Error | Fault | No |
| 17 | Alignment Check | Fault | Sí |
| 18 | Machine Check | Abort | No |
| 19 | SIMD Floating-Point | Fault | No |
| 20-31 | Reserved | - | - |
| 32-47 | IRQ 0-15 | Interrupt | No |

### Manejo de ISR en Assembly

```nasm
; Macro para ISR sin error code
%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    cli              ; Deshabilitar interrupciones
    push byte 0      ; Push dummy error code
    push byte %1     ; Push ISR number
    jmp isr_common_stub
%endmacro

; Macro para ISR con error code
%macro ISR_ERRCODE 1
  global isr%1
  isr%1:
    cli              ; Deshabilitar interrupciones
    push byte %1     ; Push ISR number
    jmp isr_common_stub
%endmacro
```

**Stack Frame en ISR**:

```
[High Address]
    Error Code (si aplica)
    ISR Number
    EIP (Instruction Pointer)
    CS (Code Segment)
    EFLAGS
[Low Address]
```

### Common Stub

```nasm
isr_common_stub:
    pusha           ; Push edi,esi,ebp,esp,ebx,edx,ecx,eax
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10     ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call isr_handler ; C handler
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8       ; Limpiar error code y ISR number
    iret             ; Return from interrupt
```

---

## VGA - Programación del Hardware

### Registro de Control CRT

```
Puerto 0x3B4 (Monocromo) / 0x3D4 (Color): Index Register
Puerto 0x3B5 (Monocromo) / 0x3D5 (Color): Data Register
```

### Registros Importantes

| Index | Registro | Descripción |
|-------|----------|-------------|
| 0x0E | Cursor Location High | Byte alto de cursor |
| 0x0F | Cursor Location Low | Byte bajo de cursor |

### Memory-Mapped I/O

```
Dirección base: 0xB8000
Resolución: 80x25 caracteres
Bytes por carácter: 2 (1 byte ASCII + 1 byte atributo)
```

### Atributo de Color (Byte bajo)

```
Bit 7: Blinking (0 = sin blink, 1 = blink)
Bit 6-4: Background color (0-15)
Bit 3: Bright text (para ciertos modos)
Bit 2-0: Foreground color (0-15)
```

### Paleta VGA de 16 Colores

| Valor | Color | Valor | Color |
|-------|-------|-------|-------|
| 0 | Black | 8 | Dark Grey |
| 1 | Blue | 9 | Light Blue |
| 2 | Green | 10 | Light Green |
| 3 | Cyan | 11 | Light Cyan |
| 4 | Red | 12 | Light Red |
| 5 | Magenta | 13 | Light Magenta |
| 6 | Brown | 14 | Light Brown |
| 7 | Light Grey | 15 | White |

**Cálculo de Atributo**:

```c
unsigned char color = (bg << 4) | (fg & 0x0F);
unsigned short entry = ascii_char | (color << 8);
```

### Scroll Implementation

```c
static void vga_scroll(void) {
    /* Mover todas las líneas hacia arriba */
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Limpiar última línea */
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; 
         i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = ' ' | (current_color << 8);
    }
    
    cursor_y = VGA_HEIGHT - 1;
}
```

---

## Convenciones de Llamada

### cdecl (C Declaration)

Usado para funciones en C del kernel:

```
Caller-saved: EAX, ECX, EDX
Callee-saved: EBX, ESI, EDI, EBP, ESP
Return value: EAX (32-bit) o EAX:EDX (64-bit)
```

**Orden de argumentos**:
```
Empuja argumentos de derecha a izquierda
Caller limpia el stack
```

### stdcall (Standard Call)

**Orden de argumentos**:
```
Empuja argumentos de derecha a izquierda
Callee limpia el stack
```

### fastcall

Primeros dos argumentos en ECX y EDX, resto en stack.

### Registro al entrar en ISR

Stack frame completo:
```
[High]
    SS (si cambio de ring)
    ESP
    EFLAGS
    CS
    EIP
    Error Code (si aplica)
    ISR Number
    EAX, ECX, EDX, EBX (por pusha)
    ESP, EBP, ESI, EDI (por pusha)
    DS, ES, FS, GS
[Low]
```

---

## Formatos de Archivos

### ELF (Executable and Linkable Format)

#### ELF Header (52 bytes)

```c
typedef struct {
    unsigned char e_ident[16];    // Magic number, etc.
    unsigned short e_type;       // Object file type
    unsigned short e_machine;    // Architecture
    unsigned int   e_version;    // Object file version
    unsigned int   e_entry;      // Entry point address
    unsigned int   e_phoff;       // Program header offset
    unsigned int   e_shoff;       // Section header offset
    unsigned int   e_flags;       // Processor-specific flags
    unsigned short e_ehsize;      // ELF header size
    unsigned short e_phentsize;   // Program header entry size
    unsigned short e_phnum;       // Program header count
    unsigned short e_shentsize;   // Section header entry size
    unsigned short e_shnum;       // Section header count
    unsigned short e_shstrndx;    // Section header string table index
} Elf32_Ehdr;
```

#### Secciones Típic

| Nombre | Descripción |
|--------|-------------|
| .text | Código ejecutable |
| .data | Datos inicializados |
| .bss | Datos no inicializados |
| .rodata | Datos read-only |
| .symtab | Símbolos |
| .strtab | Strings |

### Script del Linker

```
ENTRY(_start)

SECTIONS
{
    . = 1M;                     /* Kernel se carga a 1MB */
    
    .text : ALIGN(4K) {
        *(.multiboot)          /* Multiboot header primero */
        *(.text)               /* Código */
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata)             /* Read-only data */
    }
    
    .data : ALIGN(4K) {
        *(.data)               /* Datos inicializados */
    }
    
    .bss : ALIGN(4K) {
        *(COMMON)              /* BSS */
        *(.bss)
    }
}
```

---

## Constantes y Definiciones

### Colores VGA

```c
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN  10
#define VGA_COLOR_LIGHT_CYAN   11
#define VGA_COLOR_LIGHT_RED    12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN  14
#define VGA_COLOR_WHITE        15
```

### Dimensiones VGA

```c
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
```

### Constantes de Memoria

```c
#define KERNEL_BASE       0x100000   // 1MB
#define KERNEL_STACK_SIZE 0x4000    // 16KB
```

### Constantes Multiboot

```c
#define MULTIBOOT_MAGIC   0x2BADB002
#define MULTIBOOT_HEADER_MAGIC 0xE85250D6
```

### Segment Selectors

```c
#define KERNEL_CS 0x08   // Selector de código kernel
#define KERNEL_DS 0x10   // Selector de datos kernel
#define USER_CS   0x18   // Selector de código user
#define USER_DS   0x20   // Selector de datos user
```

---

## Referencias

### Especificaciones
- Intel® 64 and IA-32 Architectures Software Developer's Manual
- AMD64 Architecture Programmer's Manual
- Multiboot Specification (GNU GRUB)
- ELF Specification (Tool Interface Standard)

### Recursos Online
- OSDev Wiki: https://wiki.osdev.org/
- Bare Bones Tutorial: https://wiki.osdev.org/Bare_Bones
- Interrupts: https://wiki.osdev.org/Interrupts
- GDT: https://wiki.osdev.org/Global_Descriptor_Table
- IDT: https://wiki.osdev.org/Interrupt_Descriptor_Table

---

**Versión**: Fase 1
**Fecha**: Enero 2025
**Mantenedor**: SYNAPSE SO Team
