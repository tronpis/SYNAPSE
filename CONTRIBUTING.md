# SYNAPSE SO - Guía de Contribución

¡Gracias por tu interés en contribuir a SYNAPSE SO! Este documento proporciona las pautas para contribuir al proyecto.

## Código de Conducta

- Ser respetuoso y constructivo en todas las interacciones
- Aceptar críticas constructivas con gracia
- Enfocarse en lo que mejor para el proyecto
- Ayudar a otros contribuidores cuando sea posible

## ¿Cómo Contribuir?

### 1. Reportar Issues

Antes de crear un issue:
1. Busca si ya existe un issue similar
2. Proporciona un título claro y descriptivo
3. Incluye detalles suficientes para reproducir el problema
4. Usa etiquetas apropiadas (bug, enhancement, question, etc.)

### 2. Sugerir Mejoras

- Explica claramente la mejora propuesta
- Justifica por qué es necesaria
- Considera el impacto en el proyecto
- Discute en el issue tracker antes de implementar

### 3. Enviar Pull Requests

#### Pasos para enviar un PR:

1. **Fork el repositorio**
2. **Crear una rama** para tu trabajo:
   ```bash
   git checkout -b feature/tu-feature-o-fix
   ```
3. **Realiza tus cambios** siguiendo las convenciones del código
4. **Compila y prueba** el kernel:
   ```bash
   make clean
   make
   make run
   ```
5. **Commitea tus cambios** con mensajes claros:
   ```bash
   git commit -m "Tipo: descripción breve del cambio"
   ```
   Tipos permitidos: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

6. **Push a tu fork**:
   ```bash
   git push origin feature/tu-feature-o-fix
   ```

7. **Crea el Pull Request** explicando:
   - El propósito del cambio
   - Qué problemas resuelve
   - Cómo se ha probado
   - Referencias a issues relacionados

## Convenciones de Código

### Lenguajes
- **C**: Para la mayoría del código del kernel
- **Assembly**: Para código de bajo nivel (boot, ISRs)
- **Make**: Para el sistema de construcción

### Estilo de Código

#### C
- Indentación: 4 espacios (sin tabs)
- Largo de línea: máximo 80 caracteres
- Nombres de funciones: `snake_case`
- Nombres de tipos: `snake_case` o `PascalCase` para tipos definidos
- Nombres de constantes: `UPPER_CASE`

Ejemplo:
```c
/* Función bien formateada */
void example_function(int parameter) {
    if (parameter > 0) {
        do_something();
    } else {
        do_other_thing();
    }
}

/* Estructura bien formateada */
typedef struct {
    int field_one;
    unsigned int field_two;
} example_struct_t;
```

#### Assembly
- Indentación: 8 espacios
- Comentarios: `;` al inicio
- Labels: alineados a la izquierda
- Instrucciones: con indentación

Ejemplo:
```nasm
section .text
global _start

_start:
    mov eax, 1
    int 0x80
    ret
```

### Documentación
- Cada función debe tener un comentario breve explicando su propósito
- Código complejo debe tener comentarios detallados
- Usar `/* */` para comentarios multi-línea
- Usar `//` solo para comentarios de una línea en C++

## Proceso de Revisión

### Análisis Automático
Todos los PRs son analizados automáticamente por:
- Verificación de estilo de código
- Compilación cruzada si aplica
- Tests unitarios (cuando estén disponibles)
- Análisis estático de seguridad

### Revisión Manual
El mantenedor revisará:
- **Arquitectura**: ¿El cambio encaja en la visión del proyecto?
- **Seguridad**: ¿Hay vulnerabilidades o riesgos?
- **Rendimiento**: ¿Impacta negativamente el rendimiento?
- **Legibilidad**: ¿El código es claro y mantenible?
- **Documentación**: ¿Está bien documentado?

### Criterios de Aceptación
Un PR es aceptado cuando:
- ✅ Compila sin warnings
- ✅ Pasa todos los tests
- ✅ Sigue las convenciones de código
- ✅ Está bien documentado
- ✅ Mejora el proyecto o corrige un bug
- ✅ Tiene aprobación del mantenedor
- ✅ Ha pasado el análisis automático

## Prioridades del Proyecto

Las contribuciones se priorizan según:

1. **Críticas**: Bugs de seguridad, bloqueos del sistema
2. **Alta**: Funcionalidades principales del roadmap
3. **Media**: Mejoras de rendimiento, mejoras de UX
4. **Baja**: Cosméticos, refactorings no críticos

## Áreas de Contribución Sugeridas

### Para Principiantes
- Documentación y comentarios
- Tests unitarios
- Corrección de typos y errores menores
- Pequeñas refactorings

### Para Intermedios
- Implementación de drivers simples
- Mejoras en el sistema de archivos
- Optimizaciones de rendimiento
- Herramientas de desarrollo

### Para Avanzados
- Scheduling y gestión de procesos
- Drivers complejos (GPU, red)
- Sistema de memoria avanzado
- Seguridad y hardening

## Licencia

Al contribuir, aceptas que tu código sea licenciado bajo GPLv3, la misma licencia del proyecto.

## Comunicación

- **Issues**: Para reportar bugs y discutir features
- **Pull Requests**: Para proponer cambios
- **Discusiones**: Para temas arquitectónicos

## Ayuda Necesaria

Si necesitas ayuda:
1. Revisa la documentación existente
2. Busca en los issues
3. Crea un issue con la etiqueta `question`
4. Sé paciente con las respuestas

---

¡Gracias por contribuir a SYNAPSE SO! Tu ayuda hace que este proyecto sea mejor para todos.
