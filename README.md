# SYNAPSE SO Open Source
Visión: Un sistema operativo moderno, ligero y eficiente, optimizado para gaming, privacidad y compatibilidad multiplataforma. El proyecto está diseñado para ser auditable, colaborativo y seguro, utilizando herramientas automáticas y revisión manual para garantizar calidad.

Objetivos del Proyecto:
- Crear un sistema operativo open source con enfoque en gaming, privacidad y rendimiento.
- Lograr que sea ligero y eficiente, capaz de correr en hardware de baja gama, alta gama y servidores.
- Facilitar compatibilidad multiplataforma mediante capas de compatibilidad y estándares abiertos.
- Priorizar seguridad, auditoría continua y detección temprana de vulnerabilidades.
- Documentar todo el sistema con diagramas y gráficos claros para facilitar contribuciones.

Estado Actual:
**Fase 1** ✅ COMPLETADA (Enero 2025) - Boot mínimo y kernel inicial.
El kernel actual incluye:
- Bootloader compatible con Multiboot (GRUB)
- Implementación inicial del kernel en C y Assembly
- Driver VGA para modo texto
- GDT (Global Descriptor Table) configurado
- IDT (Interrupt Descriptor Table) con handlers básicos
- Sistema de construcción con Makefile
- Imagen ISO bootable

**Fase 2** ✅ COMPLETADA CON MEJORAS (Enero 2025) - Gestión de memoria y scheduler.
El kernel actual incluye:
- Physical Memory Manager (PMM) con bitmap
- Virtual Memory Manager (VMM) con paginación
- Kernel Heap con kmalloc/kfree
- Process Management con PCB completo
- Scheduler Round-Robin con quantum
- Timer Driver (PIT 8254) implementado y funcionando
- ELF Loader mejorado con copia entre espacios de direcciones
- Context Switching integrado y funcionando
- 5 correcciones críticas aplicadas
- Mejoras: mapeos temporales, validación completa, manejo de errores

**Fase 3** ✅ COMPLETADA (Enero 2025) - User Mode & System Calls
El kernel actual incluye:
- User Mode (ring 3) implementado completamente
- Transición kernel → user mode con enter_usermode()
- Proceso de usuario de prueba funcional
- System Call Interface (int 0x80) desde user mode
- Syscalls seguros con validación de punteros
- Syscalls funcionales: exit, write, getpid
- Separación de privilegios (ring 0 vs ring 3)
- Protección de memoria con PAGE_USER
- Integración completa con el scheduler
- Syscalls stub: read, open, close, fork, exec, wait
- Pendientes: fork/exec completos, filesystem, IPC

El repositorio contiene documentación completa, código fuente del kernel, y herramientas para construir y ejecutar el sistema operativo.

## Construcción y Ejecución

### Requisitos Previos
- GCC con soporte de 32 bits
- NASM (assembler)
- GNU LD (linker)
- GRUB mkrescue (para crear ISO)
- QEMU (para probar, opcional)

### Instrucciones de Construcción

```bash
# Clonar el repositorio
git clone <repository-url>
cd synapse-so

# Construir el kernel
make

# Ejecutar en QEMU
make run

# Ejecutar con modo debug
make debug

# Limpiar archivos de construcción
make clean
```

El comando `make` generará:
- `build/kernel.elf` - El kernel compilado
- `synapse.iso` - Imagen ISO bootable

### Pruebas

El kernel puede probarse de varias maneras:
1. **Virtualización**: Usar QEMU con `make run`
2. **Hardware Real**: Grabar la ISO en un USB o CD y bootear
3. **Depuración**: Usar `make debug` para ver información detallada

### Estructura del Proyecto

```
synapse-so/
├── boot/              # Código de arranque
│   ├── boot.asm      # Punto de entrada del bootloader
│   └── linker.ld     # Script del linker del kernel
├── kernel/           # Código del kernel
│   ├── include/      # Headers del kernel
│   ├── arch/x86_64/  # Código específico de arquitectura
│   ├── lib/          # Librerías del kernel
│   ├── kernel.c      # Entrada principal del kernel
│   ├── vga.c         # Driver de pantalla
│   ├── gdt.c         # Tabla de descriptores global
│   ├── idt.c         # Tabla de descriptores de interrupciones
│   └── isr.asm       # Rutinas de servicio de interrupciones
├── docs/             # Documentación técnica
├── Makefile          # Sistema de construcción
├── README.md         # Este archivo
└── LICENSE           # Licencia GPLv3
```

Para más detalles técnicos, consultar `docs/ARCHITECTURE.md`.

Contribuciones:
Cualquier persona puede contribuir mediante pull requests. Se recomienda abrir primero un issue explicando la propuesta. Todas las contribuciones serán revisadas mediante herramientas automáticas de análisis y auditoría, y la decisión final de integración corresponde al mantenedor del proyecto.

Flujo de Revisión:
- Análisis automático de errores actuales y potenciales.
- Clasificación de problemas según gravedad.
- Generación de reportes, issues y resúmenes de cada pull request.
- Revisión final humana antes del merge.

Diagramas del Sistema:
El funcionamiento del sistema operativo se documenta mediante diagramas de flujo y gráficos de arquitectura generados automáticamente. Estos gráficos se convierten a formato PNG y se incluyen en este README para mostrar claramente cómo opera el sistema internamente, incluyendo flujo de datos, dependencias y módulos principales.

Seguridad y Auditoría:
El proyecto prioriza la seguridad. Se utilizan herramientas automáticas para detectar vulnerabilidades, malas prácticas y posibles fallos futuros. Ningún cambio crítico se integra sin revisión. El objetivo es minimizar riesgos y facilitar auditorías públicas constantes.

Roadmap Inicial:
- ✅ Fase 0: Documentación base, licencia y estructura del repositorio.
- ✅ Fase 1: Boot mínimo y kernel inicial.
- ✅ Fase 2: Gestión de memoria, scheduler básico y soporte ELF.
- ✅ Fase 3: User mode, system calls y separación de privilegios.
- ⬜ Fase 4: File system, fork/exec y dynamic loading.
- ⬜ Fase 5: Optimización para videojuegos y gráficos modernos.
- ⬜ Fase 6: Hardening de seguridad, perfiles para servidor y desktop.

Licencia:
Este proyecto está licenciado bajo la GNU General Public License v3.0 (GPLv3). El uso, modificación y redistribución están permitidos bajo los términos de dicha licencia.

Nota del Mantenedor:
Se utilizan herramientas automáticas para acelerar revisiones y auditorías, pero todas las decisiones finales sobre arquitectura, seguridad y merges son responsabilidad del mantenedor. El proyecto se evalúa por su calidad técnica y transparencia.
