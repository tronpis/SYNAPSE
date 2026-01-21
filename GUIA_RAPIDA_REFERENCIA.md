# SYNAPSE SO - Guía Rápida de Referencia

## Fecha: Enero 2025

---

## 🎯 Propósito

Esta guía proporciona una referencia rápida para desarrolladores que trabajan en SYNAPSE SO, cubriendo las fases 1, 2 y 3.

---

## 📁 Estructura de Archivos

```
/home/engine/project/
├── boot/
│   ├── boot.asm          # Bootloader Multiboot
│   └── linker.ld         # Script del linker del kernel
├── kernel/
│   ├── include/kernel/   # Headers
│   │   ├── vga.h
│   │   ├── gdt.h
│   │   ├── idt.h
│   │   ├── pmm.h
│   │   ├── vmm.h
│   │   ├── heap.h
│   │   ├── process.h
│   │   ├── scheduler.h
│   │   ├── timer.h
│   │   ├── elf.h
│   │   ├── syscall.h    # NUEVO FASE 3
│   │   ├── string.h
│   │   └── io.h
│   ├── lib/
│   │   └── string.c      # Biblioteca de strings
│   ├── kernel.c          # Entry point del kernel
│   ├── vga.c             # Driver VGA
│   ├── gdt.c             # GDT
│   ├── idt.c             # IDT + handlers
│   ├── isr.asm            # ISRs + syscall stub
│   ├── pmm.c             # PMM
│   ├── vmm.c             # VMM + temp mappings
│   ├── heap.c            # Kernel heap
│   ├── process.c         # Process management
│   ├── scheduler.c       # Scheduler
│   ├── timer.c           # PIT timer
│   ├── elf.c             # ELF loader
│   ├── syscall.c         # NUEVO FASE 3
│   └── switch.asm        # Context switching
├── docs/                 # Documentación
├── Makefile             # Sistema de construcción
└── *.md                 # Documentos varios
```

---

## 🔨 Comandos de Construcción

### Básicos

```bash
# Limpiar y construir
make clean && make

# Ejecutar en QEMU
make run

# Ejecutar con debug
make debug

# Ver tamaño del kernel
make size
```

### Verificación

```bash
# Verificar herramientas
make check-tools

# Mostrar ayuda
make help
```

### Depuración

```bash
# Ejecutar con GDB
make gdb

# En otra terminal:
gdb build/kernel.elf
(gdb) target remote :1234
```

---

## 📝 Convenciones de Código

### C

```c
/* Indentación: 4 espacios (NO tabs) */
void function_name(void) {
    int local_var;
    
    /* Constantes: UPPER_CASE */
    #define CONSTANT_VALUE 0x1000
    
    /* Funciones: snake_case */
    int another_function(int param);
    
    /* Tipos: snake_case o PascalCase con _t suffix */
    typedef struct {
        int field_name;
    } custom_type_t;
}
```

### Assembly

```asm
; Indentación: 8 espacios para instrucciones
; Labels: alineados a izquierda
; Comentarios: ; al inicio de línea

function_name:
    mov eax, [ebp+8]  ; Descripción de operación
    add eax, 10
    ret

; Sección .note.GNU-stack obligatoria
section .note.GNU-stack noalloc noexec nowrite progbits
```

### Makefile

```makefile
# Variables: UPPERCASE
KERNEL_DIR = kernel
CFLAGS = -m32 -O2 -Wall

# Targets: indentación con TAB (NO espacios)
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Reglas explícitas (NO patrones ambiguos)
$(BUILD_DIR)/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@
```

---

## 🧩 Arquitectura del Kernel

### Flujo de Boot

```
BIOS/UEFI
  ↓
GRUB (Multiboot)
  ↓
boot.asm
  ↓
kernel_main()
  ↓
Inicialización secuencial:
  1. Validación de Multiboot
  2. GDT
  3. IDT
  4. PMM
  5. Heap pre-paginación
  6. VMM (habilitar paginación)
  7. Heap apropiado
  8. Process Management
  9. Scheduler
 10. Timer (PIT 8254)
  11. System Calls (Fase 3)
  12. Habilitar interrupciones
  ↓
Idle loop (hlt)
```

### Interrupciones

```
Hardware IRQ (IRQ0)
  → isr_common_stub
  → isr_handler()
    → timer_increment_tick()
    → scheduler_tick() → (cambio de contexto)
  → isr_common_stub (ajusta ESP)
  → iret
  → Próximo proceso

System Call (int 0x80)
  → isr_syscall
  → isr_common_stub
  → isr_handler()
    → syscall_handler()
    → syscall_table[num](args)
    → EAX = return value
  → isr_common_stub
  → iret
  → Continuar ejecución
```

---

## 💾 Gestión de Memoria

### Asignación de Memoria

```c
/* Kernel heap (dinámico) */
void* ptr = kmalloc(1024);
kfree(ptr);

/* Physical frames */
uint32_t phys = pmm_alloc_frame();  // Devuelve 0 si falla
pmm_free_frame(phys);

/* Virtual pages */
vmm_map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
vmm_unmap_page(virt_addr);

/* Temporary mapping (Fase 3) */
uint32_t temp = vmm_map_temp_page(phys_addr);
/* Usar temp... */
vmm_unmap_temp_page(temp);
```

### Layout de Memoria

```
Física:
  0x00000000 - 0x000FFFFF: BIOS/IVT/BDA (reservado)
  0x00100000 - 0x00FFFFFF: Kernel y datos (1MB - 16MB)
  0x01000000 - 0xFFFFFFFF: Espacio de usuario

Virtual:
  0x00000000 - 0x3FFFFFFF: Espacio de usuario (1GB)
  0xC0000000 - 0xC0400000: Heap del kernel (1MB)
  0xC0500000 - 0xC0600000: Heap temporal (1MB)
  0xE0000000 - 0xE1000000: Mapeos temporales (1MB) - NUEVO
```

---

## 🔄 Gestión de Procesos

### Crear Proceso

```c
/* Proceso kernel */
process_create("name", PROC_FLAG_KERNEL, entry_function);

/* Proceso usuario (pendiente Fase 3 completo) */
process_t* proc = process_create("name", PROC_FLAG_USER, 0);
/* Necesita: ELF loader, stack de usuario, etc.*/
```

### Scheduler

```c
/* Round-Robin automático (via timer) */
/* El scheduler cambia procesos automáticamente cada quantum ticks */

/* Forzar cambio (voluntario) */
schedule();

/* Obtener número de procesos listos */
uint32_t count = scheduler_get_ready_count();
```

---

## 📞 System Calls

### Uso desde Assembly

```asm
; exit(code)
mov eax, 1         ; SYS_EXIT
mov ebx, [code]   ; Exit code en EBX
int 0x80            ; System call

; write(fd, buffer, count)
mov eax, 2         ; SYS_WRITE
mov ebx, [fd]      ; File descriptor
mov ecx, [buffer]  ; Dirección de buffer
mov edx, [count]   ; Cuenta de bytes
int 0x80            ; System call
; EAX contiene número de bytes escritos
```

### Uso desde C

```c
/* Wrapper de syscall para exit */
void exit(int code) {
    asm volatile(
        "int $0x80"
        :
        : "a"(1), "b"(code)
    );
}

/* Wrapper de syscall para write */
int write(int fd, const void* buffer, size_t count) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(2), "b"(fd), "c"(buffer), "d"(count)
    );
    return ret;
}

/* Syscalls disponibles */
int sys_exit(int exit_code);
int sys_write(int fd, void* buffer, int count);
int sys_read(int fd, void* buffer, int count);
int sys_open(char* filename, int flags, int mode);
int sys_close(int fd);
int sys_fork(void);
int sys_exec(char* path, char** argv);
int sys_wait(int pid, int* status);
int sys_getpid(void);
```

### Convenciones de Registros

```
EAX: Número de syscall / Valor de retorno
EBX: Argumento 1
ECX: Argumento 2
EDX: Argumento 3
ESI: Argumento 4
EDI: Argumento 5
```

---

## ⚠️ Errores Comunes a Evitar

### Errores de Código

1. **NO**: Olvidar recargar CS después de cargar GDT
   - SÍ: Usar salto lejano (far jump)

2. **NO**: Apuntar entradas IDT a funciones C
   - SÍ: Usar stubs de assembly (isr_X)

3. **NO**: Usar números mágicos
   - SÍ: Usar constantes con nombres descriptivos

4. **NO**: Usar espacios en recetas de Makefile
   - SÍ: Usar TAB indentation

5. **NO**: Crear reglas de patrón ambiguas
   - SÍ: Usar reglas explícitas para cada archivo

### Errores de Memoria

6. **NO**: Calcular dirección CR3 con aritmética
   - SÍ: Usar dirección física guardada de pmm_alloc_frame()

7. **NO**: No validar ELF data size
   - SÍ: Validar todos los tamaños antes de acceder

8. **NO**: No verificar pmm_alloc_frame() retorno
   - SÍ: Siempre verificar si es 0 (fallo)

9. **NO**: Cambiar page directory sin guardar/restaurar
   - SÍ: Guardar old_dir, cambiar, restaurar en cada operación

10. **NO**: Olvidar .note.GNU-stack en assembly
    - SÍ: Agregar sección en cada archivo .asm

---

## 🔧 Depuración

### Salidas de Depuración

```bash
# Ejecutar con output de debug
make debug

# Habilitar más verbose en código
vga_print("[DEBUG] Mensaje\n");
vga_print_hex(valor);
```

### Manejo de Errores

```c
/* Siempre verificar retornos de funciones críticas */
uint32_t phys = pmm_alloc_frame();
if (phys == 0) {
    vga_print("[-] Error: No hay memoria física!\n");
    /* Manejar error gracefulmente */
}

/* Validar punteros antes de usar */
if (ptr == 0) {
    vga_print("[-] Error: Puntero nulo!\n");
    return -1;
}
```

---

## 📊 Métricas Útiles

### Comandos

```bash
# Ver tamaño del kernel
make size

# Ver símbolos del kernel
nm build/kernel.elf | less

# Ver dependencias
ldd build/kernel.elf
```

### Estadísticas del Sistema

```c
/* Memoria física */
uint32_t free_frames = pmm_get_free_frames();
uint32_t used_frames = pmm_get_used_frames();

/* Heap del kernel */
uint32_t heap_total = heap_get_total_size();
uint32_t heap_used = heap_get_used_size();
uint32_t heap_free = heap_get_free_size();

/* Scheduler */
uint32_t ready_count = scheduler_get_ready_count();
uint32_t quantum = scheduler_get_quantum();

/* Timer */
uint32_t ticks = timer_get_ticks();
```

---

## 📚 Documentación

### Documentación Técnica

1. **docs/ARCHITECTURE.md** - Arquitectura del sistema
2. **docs/TECHNICAL_REFERENCE.md** - Referencia técnica
3. **docs/DEVELOPMENT.md** - Guía para desarrolladores
4. **docs/ROADMAP.md** - Roadmap del proyecto

### Documentación de Fases

5. **docs/PHASE1_SUMMARY.md** - Resumen de Fase 1
6. **PHASE2_STATUS.md** - Estado de Fase 2
7. **PHASE2_CRITICAL_FIXES.md** - Correcciones críticas
8. **PHASE1_PHASE2_REVIEW.md** - Revisión de fases 1 y 2

### Documentación de Mejoras

9. **PHASE1_2_3_IMPROVEMENTS.md** - Mejoras (inglés)
10. **RESUMEN_MEJORAS_FASES_1_2_3.md** - Mejoras (español)
11. **PROYECTO_COMPLETO.md** - Estado del proyecto

---

## 🚀 Flujo de Desarrollo Recomendado

### Para una Nueva Característica

1. **Planeación**
   - Definir objetivos claros
   - Identificar dependencias
   - Estimar tiempo

2. **Implementación**
   - Escribir código siguiendo convenciones
   - Agregar comentarios descriptivos
   - Manejar errores gracefulmente

3. **Testing**
   - Compilar sin warnings
   - Probar funcionalidad
   - Probar casos borde

4. **Documentación**
   - Actualizar documentación técnica
   - Agregar ejemplos de uso
   - Documentar limitaciones

5. **Integración**
   - Actualizar Makefile
   - Integrar con componentes existentes
   - Probar integración

---

## 🎓 Recursos de Aprendizaje

### Documentación de Referencia

- Intel® 64 and IA-32 Architectures Software Developer's Manual
- The Little OS Book (Brandon Foltz)
- Writing a Simple OS from Scratch (Nick Blundell)
- OSDev Wiki (https://wiki.osdev.org/)

### Especificaciones

- Multiboot Specification
- ELF Specification (Tool Interface Standard)
- ext2 Filesystem Specification
- POSIX Specifications (IEEE Std 1003.1)

### Códigos de Referencia

- Minix (sistema operativo simple)
- Linux kernel (referencias de IPC, scheduler)
- xv6 (sistema operativo educativo)
- ToaruOS (OS moderno bien documentado)

---

## ✅ Checklist de Calidad

### Antes de Commit

- [ ] Código compila sin errores
- [ ] Código compila sin warnings
- [ ] Todos los archivos tienen licencia GPLv3
- [ ] Código sigue convenciones de estilo
- [ ] Sin números mágicos (usar constantes)
- [ ] Sin comentarios TODO/FIXME sin explicación
- [ ] Documentación actualizada
- [ ] Código probado en QEMU

### Para Funciones Críticas

- [ ] Validación de parámetros
- [ ] Verificación de punteros NULL
- [ ] Manejo de errores
- [ ] Verificación de asignación de memoria
- [ ] Validación de límites de buffer
- [ ] Protección contra integer overflow

---

## 🎯 Objetivos Actuales

### Fase 3 - Prioridad 1

1. **fork() real** - Proceso hijo es copia exacta
2. **exec() completo** - Reemplazar proceso con ELF
3. **wait()** - Esperar terminación de hijo
4. **Modo usuario** - Procesos en ring 3

### Fase 3 - Prioridad 2

5. **Sistema de archivos** - VFS + filesystem simple
6. **Syscalls I/O** - read, open, close completos
7. **Scheduler mejorado** - Prioridades, sleep

### Fase 3 - Prioridad 3

8. **Más syscalls** - kill, pipe, dup2, etc.
9. **IPC básico** - Pipes, señales
10. **Testing** - Framework de pruebas

---

**Fecha**: Enero 2025
**Propósito**: Guía rápida de referencia para desarrolladores
**Estado**: Proyecto en excelente estado para continuar Fase 3
