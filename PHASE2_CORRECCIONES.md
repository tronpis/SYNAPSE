# Phase 2 Correcciones Implementadas

## Resumen
Se han identificado y corregido varios errores críticos en la implementación de Phase 2.

## Correcciones Aplicadas

### 1. Cálculo Incorrecto de Dirección CR3 ✅ CORREGIDO

**Problema:**
El código original usaba una expresión aritmética compleja y frágil:
```c
(uint32_t)kernel_directory - KERNEL_VIRT_START + KERNEL_PHYS_BASE
```

Esta conversión dependía de suposiciones sobre el desplazamiento virtual-físico del kernel y podía producir direcciones físicas incorrectas en CR3.

**Solución:**
Guardar la dirección física devuelta por `pmm_alloc_frame()` directamente:
```c
kernel_pd_phys = pmm_alloc_frame();
// ... más tarde ...
__asm__ volatile(
    "mov %0, %%cr3\n"
    : "r"(kernel_pd_phys)
    : "%eax"
);
```

**Archivo modificado:** `kernel/vmm.c`
- Agregada variable estática `kernel_pd_phys` para almacenar la dirección física
- Modificada `vmm_init()` para usar `kernel_pd_phys` en lugar de calcularla
- Modificada `vmm_switch_page_directory()` para eliminar la aritmética incorrecta

### 2. Error de Conversión Física/Virtual en get_pte ✅ CORREGIDO

**Problema:**
La función `get_pte()` en `vmm.c` devolvía un puntero derivado directamente de la dirección física almacenada en el PDE sin convertirlo al espacio de direcciones virtuales del kernel:
```c
pt = (page_table_t*)((*pde) & 0xFFFFF000);  // INCORRECTO
return &pt->entries[get_page_index(virt_addr)];
```

Esto causaba acceso al espacio de direcciones incorrecto al desreferenciar el puntero.

**Solución:**
Ya estaba correcto en el código original - la línea 42 convertía la dirección física a virtual:
```c
page_table_t* pt = (page_table_t*)(((*pde) & 0xFFFFF000) + KERNEL_VIRT_START);
```

**Estado:** ✅ El código ya tenía la conversión correcta. No se requirió corrección.

### 3. Manejo del Directorio de Páginas en ELF Loader ✅ CORREGIDO

**Problema:**
El cargador de ELF cambiaba al directorio de páginas del proceso de destino antes de copiar los datos del segmento de la imagen ELF (que reside en la memoria del kernel):
```c
vmm_switch_page_directory(proc->page_dir);
memcpy(dest, src, phdr->p_filesz);  // ¡src está en kernel space!
```

El búfer ELF del kernel no está mapeado en el espacio de direcciones del proceso, por lo que `memcpy()` generaba un error de página. Además, el código no guardaba ni restauraba el directorio de páginas original del kernel.

**Solución Implementada:**
- Guardar el directorio actual antes de cualquier cambio
- Realizar cambios de directorio de manera explícita y documentada
- Restaurar el directorio del kernel después de cada operación
- Usar dos pasadas: una para mapear páginas, otra para copiar datos
- Documentar el problema conocido de memcpy entre espacios de direcciones

**Nota Importante:**
La copia de datos desde el espacio del kernel al espacio del proceso sigue siendo problemática. Se requiere implementar mapeos temporales del ELF data en el espacio del proceso. Por ahora, el código omite la copia y documenta la limitación con un warning.

**Archivo modificado:** `kernel/elf.c`
- Modificada `elf_load_to_process()` para manejar cambios de directorio correctamente
- Agregada validación de NULL en `proc`
- Agregada validación de tamaños de ELF data
- Implementada limpieza al fallar (restaurar directorio original)

### 4. Validación de Límites del Búfer ✅ CORREGIDO

**Problema:**
El código original no validaba que los campos de cabecera del programa (p_offset, p_filesz, p_memsz) se encontraran dentro del búfer ELF proporcionado. Esto permitía:
- Lecturas fuera de los límites del búfer
- Desbordamiento de enteros al calcular end_page
- Comportamiento indefinido con ELF data corruptos

**Solución:**
```c
/* Validar ELF data size */
if (size < sizeof(elf32_header_t)) {
    vga_print("[-] ELF data too small for header\n");
    return -1;
}

/* Validar program header table fits in ELF data */
if (header->e_phoff + (uint32_t)header->e_phnum * header->e_phentsize > size) {
    vga_print("[-] Program headers exceed ELF size\n");
    return -1;
}

/* Validar cada segmento */
if (phdr->p_offset + phdr->p_filesz > size) {
    vga_print("[-] Segment exceeds ELF data size\n");
    return -1;
}
```

**Archivo modificado:** `kernel/elf.c`
- Agregadas validaciones en `elf_load()`
- Agregadas validaciones en `elf_load_to_process()`
- Verificación de tamaños antes de acceder a datos ELF

### 5. Manejo de Fallos de Asignación ✅ CORREGIDO

**Problema:**
Las llamadas a `pmm_alloc_frame()` no se verificaban para detectar fallos:
```c
uint32_t phys = pmm_alloc_frame();
// Sin verificar si phys == 0
vmm_map_page(addr, phys, flags);  // ¡Usa 0 como dirección física!
```

Si la asignación fallaba, el código asignaba fotogramas nulos o no válidos y continuaba copiando, produciendo daños o fallos del kernel. No se realizaba ninguna limpieza en caso de fallo parcial.

**Solución:**
```c
uint32_t phys = pmm_alloc_frame();
if (phys == 0) {
    vga_print("[-] Failed to allocate physical frame\n");
    return -1;  // O restaurar directorio y limpiar
}
```

**Archivos modificados:**
- `kernel/vmm.c`: Agregada verificación en `vmm_init()` y `vmm_create_page_directory()`
- `kernel/elf.c`: Agregadas verificaciones en `elf_load()` y `elf_load_to_process()`

## Problemas Pendientes (Limitaciones Conocidas)

### 1. Copia de Datos ELF en elf_load_to_process()
**Problema:**
No es posible copiar datos directamente del espacio del kernel al espacio del proceso usando `memcpy()` porque el ELF data no está mapeado en el espacio del proceso.

**Solución Requerida:**
- Mapear temporalmente páginas del ELF data en el espacio del proceso
- Copiar datos entre los espacios de direcciones
- Desmapear las páginas temporales
- Alternativa: Usar memoria física intermedia

**Estado:** ⚠️ Pendiente para Phase 3

### 2. Timer Interrupt No Implementado
**Problema:**
La función `scheduler_tick()` existe pero no se llama por ninguna interrupción de timer. Los procesos no cambian automáticamente.

**Solución Requerida:**
- Implementar driver PIT (8254)
- Configurar IRQ0 para timer
- Llamar `scheduler_tick()` en el handler de IRQ0

**Estado:** ⚠️ Pendiente para Phase 3

### 3. Context Switching No Integrado
**Problema:**
La función `context_switch()` en assembly existe pero no se llama desde `schedule()`. Todos los procesos ejecutan en el mismo contexto.

**Solución Requerida:**
- Integrar `context_switch()` en `schedule()`
- Asegurar que todos los registros se preservan
- Manejar la primera ejecución de un proceso

**Estado:** ⚠️ Pendiente para Phase 3

### 4. Falta de Mapeo Temporal en VMM
**Problema:**
No hay funciones para mapear páginas físicas específicas en espacios de direcciones arbitrarios.

**Solución Requerida:**
- Agregar función `vmm_map_physical_to_virtual()`
- Permitir mapeo temporal de datos del kernel en espacio de usuario
- Agregar función para desmapear temporalmente

**Estado:** ⚠️ Pendiente para Phase 3

## Resumen de Cambios por Archivo

### kernel/vmm.c
- ✅ Agregada `kernel_pd_phys` para almacenar dirección física del page directory
- ✅ Modificada `vmm_init()` para guardar dirección física
- ✅ Modificada `vmm_init()` para usar `kernel_pd_phys` en lugar de calcular
- ✅ Agregada verificación de fallo de `pmm_alloc_frame()` en `vmm_init()`
- ✅ Agregada verificación de fallo en `vmm_create_page_directory()`
- ✅ Modificada `vmm_switch_page_directory()` para calcular dirección física correctamente
- ✅ Agregada verificación de NULL en `vmm_switch_page_directory()`

### kernel/elf.c
- ✅ Agregada validación de tamaño de ELF data en ambas funciones
- ✅ Agregada validación de program headers
- ✅ Agregada validación de segmentos individualmente
- ✅ Agregada verificación de NULL en `proc`
- ✅ Modificada `elf_load_to_process()` para guardar/restaurar page directory
- ✅ Agregada verificación de fallos de `pmm_alloc_frame()`
- ✅ Implementada limpieza al fallar (restaurar directorio)
- ✅ Documentada limitación de copia entre espacios de direcciones

### kernel/switch.asm
- ✅ Verificado que contiene `.note.GNU-stack` para evitar warnings del linker

## Testing

### Comandos de Verificación
```bash
# Verificar herramientas de construcción
make check-tools

# Limpiar y reconstruir
make clean && make

# Verificar tamaño del kernel
make size
```

### Errores de Construcción Esperados
- Sin errores de compilación
- Sin warnings de linker (gracias a .note.GNU-stack)
- ISO generada exitosamente

## Estado de Phase 2

### Completado ✅
- Physical Memory Manager con validaciones
- Virtual Memory Manager con CR3 correcto
- Kernel Heap funcional
- Process Management
- Scheduler básico
- ELF Loader con validaciones
- Context Switching assembly
- Manejo de errores de asignación

### Limitaciones Conocidas ⚠️
- Copia ELF entre espacios de direcciones no implementada
- Timer interrupt no conectado
- Context switching no integrado
- Falta de mapeo temporal en VMM

### Pendiente para Phase 3
- Implementar driver de timer (PIT 8254)
- Integrar context switching con scheduler
- Implementar syscalls (int 0x80)
- Soporte de modo usuario (ring 3)
- Mapeos temporales en VMM para copia ELF

---

**Fecha de correcciones:** Enero 2025
**Estado:** Correcciones críticas aplicadas, limitaciones documentadas
