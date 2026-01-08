# SYNAPSE SO - Guía de Desarrollo

## Guía para Desarrolladores

Este documento proporciona información técnica detallada para contribuir al desarrollo del kernel de SYNAPSE SO.

## Tabla de Contenidos

1. [Configuración del Entorno](#configuración-del-entorno)
2. [Arquitectura del Sistema](#arquitectura-del-sistema)
3. [Flujo de Desarrollo](#flujo-de-desarrollo)
4. [Componentes del Kernel](#componentes-del-kernel)
5. [Convenciones de Código](#convenciones-de-código)
6. [Testing y Debugging](#testing-y-debugging)
7. [Integración Continua](#integración-continua)

---

## Configuración del Entorno

### Requisitos del Sistema

**Herramientas Esenciales**:
- GCC 9+ (con soporte multilib para 32-bit)
- NASM 2.15+ (assembler)
- GNU binutils (ld, objdump, nm)
- Make 4.0+
- Git 2.0+

**Herramientas Opcionales**:
- QEMU/KVM para virtualización
- GDB para debugging
- GRUB mkrescue para crear ISOs
- Doxygen para generación de documentación
- Valgrind para análisis de memoria

### Configuración en Ubuntu/Debian

```bash
# Instalar dependencias esenciales
sudo apt-get update
sudo apt-get install gcc-multilib nasm binutils make git

# Herramientas opcionales
sudo apt-get install qemu-system-x86 gdb grub-pc-bin xorriso
sudo apt-get install doxygen valgrind
```

### Configuración en Fedora/RHEL

```bash
# Instalar dependencias esenciales
sudo dnf install gcc-g++ nasm binutils make git

# Herramientas opcionales
sudo dnf install qemu-system-x86 gdb grub2-tools xorriso
sudo dnf install doxygen valgrind
```

### Configuración en Arch Linux

```bash
# Instalar dependencias esenciales
sudo pacman -S gcc nasm binutils make git

# Herramientas opcionales
sudo pacman -S qemu-system-x86 gdb grub xorriso
sudo pacman -S doxygen valgrind
```

### Verificación de la Instalación

```bash
# Verificar GCC
gcc --version
gcc -m32 --version  # Debe funcionar

# Verificar NASM
nasm --version

# Verificar otras herramientas
ld --version
make --version
qemu-system-x86_64 --version
```

---

## Arquitectura del Sistema

### Visión General

```
┌─────────────────────────────────────────────────┐
│              Aplicaciones Userland               │ (Fase 3+)
├─────────────────────────────────────────────────┤
│           Interface de Syscalls                  │ (Fase 3+)
├─────────────────────────────────────────────────┤
│  Scheduler  │  VFS  │  Red  │  Drivers (Fase 4+) │ (Fase 2+)
├─────────────────────────────────────────────────┤
│          Gestión de Memoria (Fase 2+)           │
│  ┌─────────┬─────────┬─────────┬─────────┐     │
│  │ Paging  │  Heap   │  Alloc  │  Mmap   │     │
│  └─────────┴─────────┴─────────┴─────────┘     │
├─────────────────────────────────────────────────┤
│         Kernel Core (Fase 1 - Completado)       │
│  ┌─────────┬─────────┬─────────┬─────────┐     │
│  │   GDT   │   IDT   │   VGA   │  ISR    │     │
│  └─────────┴─────────┴─────────┴─────────┘     │
├─────────────────────────────────────────────────┤
│           Bootloader (GRUB/Multiboot)           │
├─────────────────────────────────────────────────┤
│              Hardware x86_64                    │
└─────────────────────────────────────────────────┘
```

### Mapa de Memoria

```
0x00000000 - 0x000FFFFF:  BIOS/IVT/BDA (Reserved)
0x00100000 - 0x00FFFFFF:  Kernel (1MB - 16MB)
0x01000000 - 0xFFFFFFFF:  User space / Available
```

### Interrupciones y Excepciones

| Vector | Descripción | Handler |
|--------|-------------|---------|
| 0-19   | Exceptions  | isr0-isr19 |
| 20-31  | Reserved    | isr20-isr31 |
| 32-47  | IRQ 0-15    | (Fase 2+) |
| 48-255 | Custom      | (Fase 2+) |

---

## Flujo de Desarrollo

### Ciclo de Vida de un Cambio

```
1. Planificación
   └─→ Discusión en issue/PR

2. Desarrollo
   ├─→ Crear branch de feature
   ├─→ Implementar cambios
   └─→ Commits atómicos

3. Testing
   ├─→ Compilación local
   ├─→ Tests unitarios (Fase 2+)
   ├─→ Testing en QEMU
   └─→ Revisión manual

4. Code Review
   ├─→ Análisis automático
   ├─→ Revisión por pares
   └─→ Aprobación del mantenedor

5. Integración
   └─→ Merge a develop/master
```

### Workflow de Git

```bash
# 1. Sincronizar con upstream
git checkout main
git pull upstream main

# 2. Crear branch de feature
git checkout -b feature/nombre-feature

# 3. Hacer cambios y commitear
git add archivo.c
git commit -m "feat: descripción del cambio"

# 4. Sincronizar y rebase
git fetch upstream
git rebase upstream/main

# 5. Push y crear PR
git push origin feature/nombre-feature
# Crear Pull Request en GitHub

# 6. Después del merge
git checkout main
git pull upstream main
git branch -d feature/nombre-feature
```

### Convenciones de Commits

Formato: `<tipo>(<ámbito>): <descripción>`

**Tipos**:
- `feat`: Nueva funcionalidad
- `fix`: Corrección de bug
- `docs`: Cambios en documentación
- `style`: Formato, sin cambios lógicos
- `refactor`: Refactorización
- `test`: Agregar tests
- `chore`: Mantenimiento, build, herramientas

**Ejemplos**:
```bash
feat(vga): add scroll function
fix(gdt): correct segment limit calculation
docs(readme): update build instructions
style(kernel): format according to conventions
```

---

## Componentes del Kernel

### 1. Boot Code (`boot/`)

#### boot.asm
```assembly
; Propósito: Punto de entrada Multiboot
; Responsabilidades:
;   - Validar Multiboot header
;   - Configurar stack (16KB)
;   - Transferir control a kernel_main()
```

#### linker.ld
```
; Propósito: Script del linker
; Responsabilidades:
;   - Definir layout de memoria
;   - Alinear secciones a 4KB
;   - Configurar entry point
```

### 2. Kernel Core (`kernel/`)

#### kernel.c
```c
// Propósito: Entrada principal del kernel
// Responsabilidades:
//   - Inicializar subsistemas
//   - Validar Multiboot
//   - Mostrar información del sistema
//   - Entrar en loop idle
```

#### vga.c
```c
// Propósito: Driver VGA modo texto
// Responsabilidades:
//   - Dibujar caracteres en pantalla
//   - Manejar colores
//   - Implementar scroll
//   - Imprimir números (dec/hex)
```

#### gdt.c
```c
// Propósito: Global Descriptor Table
// Responsabilidades:
//   - Configurar segmentos de memoria
//   - Protección (ring 0 vs ring 3)
//   - Modelo plano de memoria (4GB)
```

#### idt.c
```c
// Propósito: Interrupt Descriptor Table
// Responsabilidades:
//   - Configurar handlers de interrupciones
//   - Manejar excepciones (0-31)
//   - Preparar para IRQs (32-47)
```

#### isr.asm
```assembly
; Propósito: Interrupt Service Routines
; Responsabilidades:
;   - Guardar/restore estado del CPU
;   - Llamar handlers en C
;   - Manejar errores de interrupción
```

### 3. Headers (`kernel/include/kernel/`)

#### vga.h
```c
// Defines VGA: VGA_WIDTH, VGA_HEIGHT, colores
// Funciones: vga_print, vga_set_color, etc.
```

#### gdt.h
```c
// Función: gdt_init()
```

#### idt.h
```c
// Funciones: idt_init(), isr_handler()
```

### 4. Librerías (`kernel/lib/`)

#### string.c
```c
// Funciones de strings: strlen, strcmp, strcpy
```

---

## Convenciones de Código

### C

#### Formato
```c
/* Indentación: 4 espacios (sin tabs) */
/* Largo de línea: máximo 80 caracteres */

/* Funciones: snake_case */
void example_function(int param);

/* Estructuras: snake_case o PascalCase con _t suffix */
typedef struct {
    int field_one;
} example_struct_t;

/* Constantes: UPPER_CASE */
#define MAX_VALUE 100

/* Variables: snake_case */
int variable_name;
```

#### Comentarios
```c
/* Single-line comment above code */
void function(void);

/*
 * Multi-line comment for
 * complex logic or explanations
 */
int complex_calculation(int x) {
    /* Inline comment for tricky part */
    return x * 2;
}
```

#### Includes
```c
/* System includes primero */
#include <stdint.h>

/* Kernel includes después */
#include <kernel/vga.h>
#include <kernel/gdt.h>

/* Local includes por último */
#include "local_header.h"
```

### Assembly

#### Formato
```nasm
; Indentación: 8 espacios
; Comments: ; al inicio

section .text
global _start

_start:
    mov eax, 1      ; syscall number
    int 0x80        ; invoke kernel
    ret             ; return
```

### Makefile

#### Formato
```makefile
# Variables en uppercase
CC = gcc
CFLAGS = -O2 -Wall

# Targets: tab indentation
target:
	$(CC) $(CFLAGS) -o $@ $^

# Comments: # al inicio
# This is a comment
```

---

## Testing y Debugging

### Testing en QEMU

```bash
# Ejecución normal
make run

# Con más recursos
qemu-system-x86_64 -cdrom synapse.iso -m 1G -smp 2

# Con serial output
qemu-system-x86_64 -cdrom synapse.iso -serial stdio

# Con GDB server
qemu-system-x86_64 -cdrom synapse.iso -s -S
# En otra terminal: gdb build/kernel.elf
# (gdb) target remote :1234
```

### Debugging con GDB

```bash
# Compilar con símbolos de debug
make CFLAGS="-g -O0"

# Ejecutar QEMU con GDB
qemu-system-x86_64 -cdrom synapse.iso -s -S &

# Conectar GDB
gdb build/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
(gdb) step  # step over
(gdb) next  # step into
(gdb) print variable_name
(gdb) info registers
(gdb) x/10i $pc  # disassemble
```

### Análisis de Código

```bash
# Ver símbolos
nm build/kernel.elf

# Disassembly
objdump -D build/kernel.elf > kernel.asm

# Ver secciones
readelf -S build/kernel.elf

# Ver headers
readelf -h build/kernel.elf

# Tamaño del kernel
size build/kernel.elf
```

### Herramientas de Análisis

```bash
# Verificar sintaxis de assembly
nasm -f elf32 -o /dev/null file.asm

# Verificar compilación (sin linker)
gcc -m32 -c -Wall -Wextra file.c

# Static analysis (si está disponible)
cppcheck kernel/

# Check memory leaks (Fase 2+)
valgrind --tool=memcheck ./test_program
```

---

## Integración Continua

### Pruebas Automáticas (Plan Fase 2+)

```makefile
# Tests unitarios
test-unit:
    ./tests/unit_tests

# Tests de integración
test-integration:
    ./tests/integration_tests

# Tests de boot
test-boot:
    timeout 30 qemu-system-x86_64 -cdrom synapse.iso
```

### Verificación Automática

Antes de cada commit, verificar:

```bash
# 1. Limpia y reconstruye
make clean && make

# 2. Verifica que compile sin warnings
make 2>&1 | grep -i warning

# 3. Prueba en QEMU
make run

# 4. Verifica tamaño del kernel
make size
```

### Checklist para PRs

Antes de enviar un PR:

- [ ] Compila sin errores ni warnings
- [ ] Pasa tests (cuando estén disponibles)
- [ ] Funciona en QEMU
- [ ] Sigue convenciones de código
- [ ] Tiene comentarios donde sea necesario
- [ ] Actualiza documentación
- [ ] Commits son atómicos y bien descriptos
- [ ] Rebase con rama principal actual

---

## Recursos Adicionales

### Documentación Externa

- **x86 Architecture**: Intel SDM, AMD APM
- **Multiboot Specification**: GNU GRUB manual
- **POSIX Standards**: IEEE Std 1003.1
- **ELF Format**: Tool Interface Standard

### Herramientas Útiles

- **hexdump**: Ver contenido binario
- **strings**: Encontrar strings en binarios
- **objdump**: Disassembly y análisis
- **readelf**: Información ELF
- **nm**: Símbolos de binarios

### Comunidad

- Issues del proyecto: Reportar bugs y discutir features
- Pull Requests: Contribuir código
- Discussions: Consultas técnicas

---

**¡Happy Hacking!** 🚀

Para preguntas o ayuda, no dudes en abrir un issue en el repositorio.
