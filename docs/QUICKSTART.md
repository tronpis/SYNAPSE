# SYNAPSE SO - Guía de Inicio Rápido

## ¿Qué es SYNAPSE SO?

SYNAPSE SO es un sistema operativo de código abierto (GPLv3) diseñado para:
- ✅ **Gaming**: Optimizado para bajo latencia y alto rendimiento
- ✅ **Privacidad**: Enfoque en seguridad y protección de datos
- ✅ **Multi-plataforma**: Compatibilidad a través de estándares abiertos
- ✅ **Ligero**: Eficiente en hardware de baja gama
- ✅ **Auditable**: Código abierto y bien documentado

## Prueba Rápida (5 minutos)

### 1. Requisitos Mínimos

```bash
# Verifica que tienes las herramientas necesarias
gcc --version      # GCC con soporte 32-bit
nasm --version     # NASM assembler
ld --version       # GNU linker
```

### 2. Construye el Kernel

```bash
# Clona el repositorio (si aún no lo has hecho)
git clone <repository-url>
cd synapse-so

# Construye todo
make

# El resultado: synapse.iso (imagen bootable)
```

### 3. Ejecuta en QEMU

```bash
# Ejecución normal
make run

# Ejecución con debug
make debug

# Con más memoria
qemu-system-x86_64 -cdrom synapse.iso -m 1G
```

## Verás Algo Como Esto

```
SYNAPSE SO - Open Source Operating System
=========================================

Initializing Kernel...
[+] Multiboot validated successfully
[+] Initializing Global Descriptor Table...
    GDT loaded successfully
[+] Initializing Interrupt Descriptor Table...
    IDT loaded successfully

System Information:
    Lower memory: 640 KB
    Upper memory: 524288 KB

[SUCCESS] Kernel initialized successfully!
SYNAPSE SO is ready.
```

## Estructura del Proyecto

```
synapse-so/
├── boot/              # Código de arranque (boot.asm, linker.ld)
├── kernel/            # Código del kernel
│   ├── include/       # Headers (vga.h, gdt.h, idt.h)
│   ├── lib/           # Librerías (string.c)
│   ├── kernel.c       # Punto de entrada del kernel
│   ├── vga.c          # Driver VGA
│   ├── gdt.c          # Gestión de memoria segmentada
│   ├── idt.c          # Gestión de interrupciones
│   └── isr.asm        # Rutinas de interrupción (assembly)
├── docs/              # Documentación
│   ├── ARCHITECTURE.md # Arquitectura técnica
│   ├── ROADMAP.md     # Roadmap detallado
│   └── QUICKSTART.md  # Este archivo
├── Makefile           # Sistema de construcción
├── README.md          # Información general
├── CONTRIBUTING.md    # Guía de contribución
└── LICENSE            # GPLv3
```

## Comandos Útiles del Makefile

```bash
make           # Construye kernel e ISO
make run       # Ejecuta en QEMU
make debug     # Ejecuta con debug
make clean     # Limpia archivos de construcción
make rebuild   # Limpia y reconstruye
make size      # Muestra tamaño del kernel
make help      # Muestra ayuda
```

## Estado Actual

### ✅ Fase 1 Completada (Actual)
- Bootloader Multiboot funcional
- Kernel básico con:
  - Driver VGA (modo texto 80x25)
  - GDT configurado (protección de memoria)
  - IDT con 32 handlers de interrupciones
  - Sistema de construcción completo

### 🔄 Próximas Fases
- **Fase 2**: Gestión de memoria, scheduler, ELF loader
- **Fase 3**: Userland POSIX, filesystem
- **Fase 4**: Gráficos, gaming optimizations
- **Fase 5**: Seguridad hardening

## ¿Cómo Contribuir?

### Para Principiantes
1. Lee `CONTRIBUTING.md`
2. Lee `docs/ARCHITECTURE.md`
3. Revisa issues abiertos
4. Comienza con tareas simples (documentación, tests)

### Para Desarrolladores
1. Familiarízate con el código
2. Elige un área de interés (ver ROADMAP.md)
3. Crea una rama para tu trabajo
4. Haz cambios siguiendo las convenciones
5. Prueba con `make run`
6. Envía un Pull Request

### Áreas que Necesitan Trabajo
- 📝 Documentación y comentarios
- 🧪 Tests unitarios
- 💾 Gestión de memoria (Fase 2)
- ⚡ Scheduler de procesos (Fase 2)
- 🖥️ Drivers de gráficos (Fase 4)
- 🔒 Implementación de seguridad (Fase 5)

## Arquitectura Básica

### Flujo de Arranque
```
BIOS/UEFI
    ↓
GRUB (Multiboot)
    ↓
boot.asm (stack setup)
    ↓
kernel_main() (C)
    ↓
Inicialización
    ├→ vga_clear_screen()
    ├→ gdt_init()
    ├→ idt_init()
    └→ display_system_info()
    ↓
Idle loop (hlt)
```

### Componentes del Kernel

#### VGA Driver (`vga.c`)
- Muestra texto en pantalla 80x25
- Soporta 16 colores (fg y bg)
- Funciones: print, print_dec, print_hex

#### GDT (`gdt.c`)
- Tabla de descriptores global
- Configura segmentos de memoria
- Kernel y user mode (ring 0 y 3)

#### IDT (`idt.c`)
- Tabla de descriptores de interrupciones
- 256 interrupt handlers
- Excepciones (div by zero, page fault, etc.)

## Desarrollo y Debugging

### Ver el Código
```bash
# Navega el código
ls boot/          # Código de arranque
ls kernel/        # Código del kernel
ls kernel/include/kernel/  # Headers
```

### Modificar y Probar
```bash
# 1. Edita un archivo
nano kernel/kernel.c

# 2. Reconstruye
make clean && make

# 3. Prueba
make run

# 4. Si funciona, commitea
git add kernel/kernel.c
git commit -m "feat: add new feature"
```

### Debugging
```bash
# Ejecuta con debug
make debug

# Ver código assembly generado
objdump -D build/kernel.elf > kernel.asm

# Ver símbolos
nm build/kernel.elf
```

## Preguntas Frecuentes

### Q: ¿En qué arquitectura corre?
A: x86_64 (32-bit kernel, puede ejecutarse en hardware de 64-bit)

### Q: ¿Puede correr en hardware real?
A: Sí, graba la ISO en un USB y bootea desde BIOS

### Q: ¿Cuándo estará listo para uso diario?
A: Estamos en Fase 1 de 5. Varios meses de desarrollo.

### Q: ¿Cómo puedo ayudar?
A: Cualquier contribución es bienvenida. Ver `CONTRIBUTING.md`

### Q: ¿Qué lenguaje de programación se usa?
A: C para el kernel, Assembly para low-level, Make para construcción

### Q: ¿Es compatible con Linux?
A: No es compatible binariamente con Linux, pero sigue estándares POSIX

## Recursos

- **Documentación**: `docs/` directory
- **Roadmap**: `docs/ROADMAP.md`
- **Arquitectura**: `docs/ARCHITECTURE.md`
- **Contribución**: `CONTRIBUTING.md`
- **Issues**: GitHub Issues del proyecto

## Licencia

Este proyecto está bajo licencia GPLv3. Código libre y abierto para todos.

---

**¡Bienvenido a SYNAPSE SO!** 🚀

Para más información, visita el README o los documentos en docs/.
