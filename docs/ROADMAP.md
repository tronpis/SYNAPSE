# SYNAPSE SO - Roadmap Detallado

## Visión General

SYNAPSE SO es un sistema operativo de código abierto diseñado para ser moderno, ligero y eficiente, con enfoque en gaming, privacidad y compatibilidad multiplataforma.

## Fases de Desarrollo

### ✅ Fase 0: Documentación Base y Estructura (COMPLETADA)
**Estado**: Completado

**Objetivos**:
- [x] Documentación del proyecto (README.md)
- [x] Licencia GPLv3 establecida
- [x] Estructura básica del repositorio
- [x] Guías de contribución
- [x] Documentación de arquitectura

**Entregables**:
- README.md con visión y objetivos
- LICENSE (GPLv3)
- CONTRIBUTING.md
- .gitignore
- Documentación técnica inicial

---

### ✅ Fase 1: Boot Mínimo y Kernel Inicial (COMPLETADA)
**Estado**: Completado

**Objetivos**:
- [x] Bootloader compatible con Multiboot
- [x] Kernel básico funcional
- [x] Driver VGA para modo texto
- [x] GDT (Global Descriptor Table)
- [x] IDT (Interrupt Descriptor Table)
- [x] Sistema de construcción (Makefile)
- [x] Imagen ISO bootable

**Entregables**:
- boot/boot.asm - Punto de entrada Multiboot
- boot/linker.ld - Script del linker
- kernel/kernel.c - Entrada principal del kernel
- kernel/vga.c - Driver de pantalla VGA
- kernel/gdt.c - Gestión de segmentación de memoria
- kernel/idt.c - Gestión de interrupciones
- kernel/isr.asm - Rutinas de servicio de interrupciones
- Makefile - Sistema de construcción completo
- synapse.iso - Imagen bootable

**Próximos pasos**: Comenzar Fase 2

---

### 🔄 Fase 2: Gestión de Memoria y Scheduler Básico (EN PROGRESO)
**Estado**: Próxima fase

**Objetivos**:
- [ ] Gestor de memoria física (frame allocator)
- [ ] Gestor de memoria virtual (paging)
- [ ] Heap del kernel (kmalloc/kfree)
- [ ] Scheduler básico (Round-Robin)
- [ ] Estructuras de procesos (PCB)
- [ ] Soporte para binarios ELF
- [ ] Cambio de contexto

**Subtareas**:

#### 2.1 Memoria Física
- [ ] Detección de memoria disponible
- [ ] Bitmap para跟踪 de frames
- [ ] Funciones: alloc_frame(), free_frame()
- [ ] Manejo de regiones reservadas

#### 2.2 Memoria Virtual
- [ ] Setup de paginación (CR3, páginas 4KB)
- [ ] Directorio y tablas de páginas
- [ ] Mapeo: map_page(), unmap_page()
- [ ] Fault handler para page faults

#### 2.3 Kernel Heap
- [ ] Algoritmo de asignación (simple o slab)
- [ ] kmalloc(), kfree(), krealloc()
- [ ] Manejo de fragmentación
- [ ] Debugging y estadísticas

#### 2.4 Scheduler
- [ ] Cola de listos (ready queue)
- [ ] Round-Robin con quantum configurable
- [ ] Cambio de contexto (save/restore state)
- [ ] Manejo de prioridades básico

#### 2.5 ELF Loader
- [ ] Parseo de cabeceras ELF
- [ ] Carga de secciones en memoria
- [ ] Relocaciones básicas
- [ ] Creación de espacio de direcciones del proceso

**Tiempo estimado**: 4-6 semanas

---

### ⬜ Fase 3: Userland Compatible POSIX
**Estado**: Pendiente

**Objetivos**:
- [ ] Interfaz de llamadas al sistema (syscalls)
- [ ] Gestión de procesos (fork, exec, exit, wait)
- [ ] Sistema de archivos VFS
- [ ] Sistema de archivos simple (ext2 o custom)
- [ ] Utilidades Unix básicas
- [ ] Shell simple (bash-compatible)

**Subtareas**:

#### 3.1 Syscalls
- [ ] Mecanismo de syscalls (int 0x80 o sysenter)
- [ ] Tabla de syscalls
- [ ] Implementación de syscalls básicas:
  - [ ] read, write, open, close
  - [ ] exit, fork, exec, wait
  - [ ] getpid, getppid
  - [ ] brk, sbrk (gestión de memoria)
  - [ ] stat, fstat
  - [ ] ioctl

#### 3.2 Procesos
- [ ] Estructura PCB completa
- [ ] Espacio de usuario y kernel
- [ ] Creación y destrucción de procesos
- [ ] Herencia de recursos
- [ ] Manejo de señales básico

#### 3.3 VFS
- [ ] Abstracción de filesystem
- [ ] Operaciones: open, read, write, seek
- [ ] Cache de archivos
- [ ] Manejo de inodes
- [ ] Montaje/desmontaje

#### 3.4 Filesystem
- [ ] Implementación básica (ext2 o custom)
- [ ] Creación de archivos/directorios
- [ ] Borrado y renombrado
- [ ] Permisos básicos
- [ ] Journaling (opcional)

#### 3.5 Userland
- [ ] Init process
- [ ] Shell simple
- [ ] Utilidades: ls, cat, echo, pwd, cd
- [ ] Editor de texto básico
- [ ] Librería C básica

**Tiempo estimado**: 6-8 semanas

---

### ⬜ Fase 4: Optimización para Gaming y Gráficos
**Estado**: Pendiente

**Objetivos**:
- [ ] Scheduler en tiempo real para gaming
- [ ] Marco de trabajo para drivers de GPU
- [ ] Subsistema gráfico básico
- [ ] Sistema de input (teclado, mouse)
- [ ] Capa de compatibilidad para APIs de juegos
- [ ] Gestión de audio

**Subtareas**:

#### 4.1 Scheduler RT
- [ ] Colas de prioridades múltiples
- [ ] Algoritmo de prioridad dinámica
- [ ] Minimización de latencia
- [ ] Asignación de CPU cores

#### 4.2 Drivers GPU
- [ ] Abstracción de hardware
- [ ] Modo framebuffer
- [ ] Aceleración 2D básica
- [ ] Soporte para modos video VESA/VBE
- [ ] Drivers para tarjetas comunes

#### 4.3 Gráficos
- [ ] Window manager básico
- [ ] Dibujo de primitivas (líneas, rects, texturas)
- [ ] Sistema de eventos de ventana
- [ ] Composición básica
- [ ] Soporte para múltiples monitores

#### 4.4 Input
- [ ] Driver de teclado (PS/2 y USB)
- [ ] Driver de mouse (PS/2 y USB)
- [ ] Sistema de eventos de input
- [ ] Soporte para gamepads básicos

#### 4.5 API de Gaming
- [ ] Abstracción de API (similar a SDL)
- [ ] Gestión de tiempo (high-resolution timers)
- [ ] Joystick/Gamepad API
- [ ] Audio básico
- [ ] Networking básico (UDP/TCP)

**Tiempo estimado**: 8-10 semanas

---

### ⬜ Fase 5: Hardening de Seguridad y Perfiles
**Estado**: Pendiente

**Objetivos**:
- [ ] ASLR (Address Space Layout Randomization)
- [ ] Stack canaries
- [ ] NX/DEP (No-Execute/Data Execution Prevention)
- [ ] Secure Boot
- [ ] Control de acceso (ACLs)
- [ ] Auditoría y logging
- [ ] Perfiles de servidor y desktop

**Subtareas**:

#### 5.1 Protecciones de Memoria
- [ ] Implementación de ASLR
- [ ] Stack canaries en el compilador
- [ ] NX bit en páginas
- [ ] ASLR para kernel
- [ ] Protección contra overflow

#### 5.2 Control de Acceso
- [ ] Sistema de usuarios y grupos
- [ ] Permisos Unix (rwx)
- [ ] ACLs extendidas
- [ ] Capabilities
- [ ] SELinux o AppArmor (opcional)

#### 5.3 Secure Boot
- [ ] Firma de kernel
- [ ] Chain of trust
- [ ] Medición de arranque
- [ ] Recovery mode

#### 5.4 Auditoría
- [ ] Sistema de logging comprehensivo
- [ ] Auditoría de syscalls
- [ ] Detección de intrusiones básica
- [ ] Reportes de seguridad
- [ ] Herramientas de análisis

#### 5.5 Perfiles
- [ ] Perfil Desktop (GUI, multimedia)
- [ ] Perfil Server (headless, networking)
- [ ] Perfil Gaming (RT scheduler, GPU focus)
- [ ] Perfil Embedded (minimal footprint)

**Tiempo estimado**: 6-8 semanas

---

## Hitos Futuros

### Meses 1-2: Base del Sistema Operativo
- Fase 2 completada
- Kernel con memoria y procesos funcionales

### Meses 3-4: Userland Funcional
- Fase 3 completada
- Sistema usable desde línea de comandos

### Meses 5-7: Gráficos y Gaming
- Fase 4 completada
- GUI básica y capacidades de gaming

### Meses 8-9: Seguridad y Producción
- Fase 5 completada
- Sistema listo para uso general

### Meses 10+: Mejoras y Características Avanzadas
- Optimización continua
- Características avanzadas
- Compatibilidad ampliada

## Prioridades del Proyecto

1. **Funcionalidad Primero**: Sistema usable antes de optimizar
2. **Seguridad Siempre**: Cada fase considera seguridad
3. **Modularidad**: Fácil de extender y mantener
4. **Documentación**: Código bien documentado para contribuciones
5. **Comunidad**: Facilitar contribuciones externas

## Contribuciones Bienvenidas

El proyecto acepta contribuciones en cualquier fase. Las áreas prioritarias son:

- **Principiantes**: Documentación, tests, UI básica
- **Intermedios**: Drivers, optimizaciones, features
- **Avanzados**: Core del kernel, seguridad, gráficos

Para más información, ver `CONTRIBUTING.md`.

---

*Última actualización: Enero 2025*
