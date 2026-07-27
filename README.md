# Documentación Técnica y Manual de Usuario: Admin_Sys 

**Admin_Sys** es una suite de administración de sistemas en modo consola desarrollada en C (C99 / POSIX). Este documento ofrece una visión integral del proyecto, cubriendo desde sus objetivos y requisitos de compilación hasta el detalle de su arquitectura modular y funcionalidades.

---

## 1. Introducción

**Admin_Sys** es una aplicación de consola desarrollada en lenguaje C que funciona como un panel de administración básica desde la terminal. Su propósito principal es integrar varias funcionalidades relacionadas con la administración de sistemas en una sola interfaz simple, intuitiva y fácil de usar.

El proyecto está orientado a usuarios y administradores que desean interactuar con tareas comunes de gestión sin necesidad de recurrir a múltiples herramientas independientes por separado. A través de un menú interactivo, el usuario puede ejecutar comandos del sistema, gestionar respaldos, analizar scripts Bash, simular descargas multihilo y revisar un historial detallado de operaciones.

Aunque el programa está concebido como una herramienta educativa y demostrativa de conceptos avanzados de C y llamadas al sistema POSIX, también sirve como una base sólida para proyectos de automatización, monitoreo o desarrollo de herramientas CLI.

---

## 2. Objetivo del proyecto

El objetivo central de **Admin_Sys** es ofrecer una interfaz de texto que centralice y organice varias funciones de utilidad para la administración de sistemas. Sus metas principales incluyen:

* **Facilitar la ejecución de comandos** externos desde una consola unificada y amigable.
* **Proporcionar un motor de respaldos** para la creación y restauración de copias de seguridad de directorios.
* **Analizar scripts Bash** de manera estática (*linter*) para auditar su estructura y detectar riesgos antes de ejecutarlos.
* **Simular un gestor de descargas** asíncrono y multihilo basado en colas de tareas.
* **Mantener una traza e historial** de las operaciones realizadas durante la sesión del usuario.

En resumen, el proyecto busca combinar capacidades operativas esenciales en una sola aplicación simple, modular y extensible.

---

## 3. Alcance del sistema

**Admin_Sys** no pretende reemplazar a un sistema operativo ni competir con suites empresariales de administración. Su alcance está delimitado al ámbito educativo, demostrativo y de uso personal o local.

### Funcionalidades dentro del alcance

* Navegación interactiva por menú textual.
* Ejecución de comandos del sistema operativo con soporte de redirección de E/S (`>` y `>>`).
* Copia de seguridad incremental y restauración de carpetas.
* Análisis sintáctico estático de archivos `.sh`.
* Manejo de cola de descargas concurrentes mediante hilos POSIX.
* Registro de operaciones con marcas de tiempo y códigos de salida.
* Cierre ordenado y liberación de recursos en RAM.

---

## 4. Características principales

### 4.1 Interfaz de consola

La aplicación se ejecuta íntegramente en la terminal (CLI). Presenta un menú principal numerado que aguarda la interacción del usuario mediante texto e instrucciones estándar.

### 4.2 Ejecución de comandos externos

Permite al usuario ingresar comandos nativos del sistema operativo y ejecutarlos directamente. Soporta la redirección de la salida estándar a archivos en disco.

### 4.3 Gestión de respaldos

Ofrece utilidades para clonar y recuperar estructuras de carpetas. Implementa lógica incremental que compara estados de archivos para transferir únicamente los elementos nuevos o modificados.

### 4.4 Análisis de scripts Bash

Examina scripts de shell sin ejecutarlos. Ofrece estadísticas de líneas de código, comentarios, variables y bucles, además de emitir alertas sobre comandos potencialmente peligrosos.

### 4.5 Simulación de descargas multihilo

Demuestra el uso de concurrencia mediante hilos en C. Maneja una cola compartida de tareas con monitoreo del progreso de descargas simuladas en tiempo real.

### 4.6 Historial de operaciones

Registra cada acción ejecutada, almacenando datos como la hora de inicio, el comando procesado, la duración de la tarea y su resultado.

---

## 5. Requisitos del sistema

Para compilar y ejecutar **Admin_Sys** se requieren las siguientes herramientas e interfaces:

| Componente | Requisito Mínimo | Propósito |
| --- | --- | --- |
| **Sistema Operativo** | Linux, macOS o Windows (vía WSL / MinGW) | Entorno de ejecución CLI con soporte POSIX |
| **Compilador** | GCC 4.8+ o Clang 3.3+ | Compilación del código fuente en estándar C99 |
| **Herramienta de Construcción** | GNU Make | Automatización del proceso de compilación modular |
| **Biblioteca Concurrente** | `libpthread` (POSIX Threads) | Manejo de hilos y primitivas de sincronización |
| **Llamadas al Sistema** | Funciones POSIX (`fork`, `exec`, `stat`, `dup2`) | Manejo de procesos, archivos y descriptores |

---

## 6. Instalación y preparación del entorno

Antes de proceder con la compilación, asegúrese de contar con los paquetes esenciales de desarrollo. En distribuciones basadas en Debian/Ubuntu, puede instalarlos ejecutando:

```bash
sudo apt update
sudo apt install build-essential

```

La estructura típica de directorios del proyecto se organiza de la siguiente manera:

```text
Admin_Sys/
├── Makefile
├── src/
│   ├── main.c
│   ├── comandos.c
│   ├── respaldos.c
│   ├── analizador_bash.c
│   ├── descargas.c
│   ├── procesos.c
│   ├── archivos.c
│   └── utils.c
├── include/
│   ├── comandos.h
│   ├── respaldos.h
│   ├── analizador_bash.h
│   ├── descargas.h
│   ├── procesos.h
│   ├── archivos.h
│   └── utils.h
├── obj/          # Carpeta generada automáticamente para archivos .o
└── bin/          # Carpeta generada automáticamente para el ejecutable

```

---

## 7. Compilación

La compilación se gestiona mediante la herramienta `make`, la cual lee las reglas declaradas en el `Makefile` para procesar los módulos de forma limpia.

### Comando de compilación automatizada

```bash
make

```

Este comando realiza las siguientes acciones:

1. Crea los directorios `obj/` y `bin/` en caso de que no existan.
2. Compila cada archivo `.c` de la carpeta `src/` generando sus correspondientes archivos objeto `.o`.
3. Enlaza los objetos con las bibliotecas del sistema (como `-pthread`) y genera el binario ejecutable final.

### Limpieza de archivos compilados

Si necesita recompilar el proyecto desde cero, utilice:

```bash
make clean

```

---

## 8. Ejecución

Una vez completada la compilación, ejecute el binario desde la raíz del proyecto:

```bash
./bin/admin_sys

```

*Nota: En entornos Windows compilados con MinGW, el ejecutable puede generarse como `./bin/admin_sys.exe`.*

---

## 9. Funcionamiento general del programa

Al iniciar **Admin_Sys**, la aplicación sigue una secuencia de arranque ordenada:

```text
 [Inicio] ──> [Cargar Configuración e Historial] ──> [Inicializar Cola de Descargas y Mutex]
                                                                  │
 ┌────────────────────────────────────────────────────────────────┘
 ▼
[Mostrar Menú Principal] <──> [Leer Opción] ──> [Ejecutar Módulo] ──> [Registrar Historial]

```

El ciclo permanece activo hasta que el usuario selecciona de manera explícita la opción de salida (`0`), la cual se encarga de liberar la memoria asignada y cerrar los recursos abiertos.

---

## 10. Menú principal

Al arrancar la aplicación se despliega la siguiente interfaz en pantalla:

```text
===================================================================
                       ADMIN_SYS — PANEL DE CONTROL
===================================================================
 1. Ejecutar comando externo / redirección
 2. Respaldos incrementales y restauración
 3. Analizador estático de scripts Bash
 4. Gestor multihilo de descargas
 5. Ver / gestionar historial de comandos
 0. Salir del sistema
===================================================================
Seleccione una opción [0-5]: 

```

---

## 11. Descripción detallada de cada opción del menú

### 11.1 Opción 1: Ejecutar comando externo / redirección

Permite ejecutar comandos binarios disponibles en el sistema operativo sin cerrar la sesión de **Admin_Sys**.

* **Funcionamiento interno:** La aplicación procesa la entrada del usuario y detecta si contiene operadores de redirección (`>` o `>>`). Utiliza `fork()` para bifurcar el proceso, `dup2()` para reorientar los descriptores de la salida estándar hacia el archivo especificado y `execvp()` para invocar el comando.
* **Modos de redirección:**
* `>` : Crea el archivo destino o lo sobrescribe si ya existe.
* `>>` : Crea el archivo destino o añade el nuevo resultado al final del archivo existente.



**Ejemplo de uso:**

```text
Ingresa el comando: ls -l /var/log > registros.txt
[OK] Comando ejecutado exitosamente. Salida redirigida a 'registros.txt'.

```

### 11.2 Opción 2: Respaldos incrementales y restauración

Permite realizar copias de seguridad eficientes entre directorios.

* **Lógica incremental:** En lugar de duplicar la totalidad de la estructura en cada ejecución, el motor inspecciona las carpetas mediante `stat()`. Compara las marcas de tiempo de modificación (`mtime`) y los tamaños en bytes. Copia únicamente aquellos archivos que no existen en el destino o cuya versión en origen es más reciente.
* **Restauración:** Invierte el flujo de trabajo, copiando los datos guardados en la carpeta de seguridad de vuelta al directorio de trabajo indicado.

**Ejemplo de uso:**

```text
[RESPALDOS INCREMENTALES]
Ruta Origen: /home/usuario/documentos
Ruta Destino: /var/backups/documentos

Procesando archivos...
 - [NUEVO]     reporte.pdf -> Copiado.
 - [SIN CAMBIO] notas.txt   -> Omitido.
Respaldo completado en 0.08 segundos.

```

### 11.3 Opción 3: Analizador estático de scripts Bash

Examina archivos `.sh` buscando patrones estructurales y posibles alertas de seguridad sin ejecutar ninguna orden en la máquina anfitriona.

* **Métricas analizadas:**
* Conteo de líneas totales, líneas de código útil, líneas vacías y comentarios (`#`).
* Declaración de variables globales y locales.
* Presencia de bucles (`for`, `while`, `until`) y definición de funciones.


* **Alertas de riesgo:** Detecta el uso de órdenes peligrosas como `rm -rf /`, evaluación dinámica no segura con `eval`, o claves/contraseñas declaradas en texto plano.

**Ejemplo de uso:**

```text
Ruta del script .sh: ./scripts/despliegue.sh

=== REPORTE DE ANÁLISIS ESTÁTICO ===
Líneas totales: 45 | Comentarios: 10 | Vacías: 5
Variables declaradas: 6 | Funciones: 2
Alertas detectadas:
 - [ADVERTENCIA - Línea 23]: Uso detectado de 'eval'.
================================────

```

### 11.4 Opción 4: Gestor multihilo de descargas

Simula la descarga simultánea de múltiples archivos mediante un modelo concurrente.

* **Funcionamiento interno:** Emplea hilos trabajadores (`pthread_create`) que consumen tareas de una cola global. La estructura compartida está protegida mediante cerrojos de exclusión mutua (`pthread_mutex_t`) para prevenir condiciones de carrera.
* **Estados de la tarea:**
* `PENDIENTE`: Registrada en cola a la espera de un hilo libre.
* `EN PROCESO`: Descarga en ejecución con actualización progresiva del porcentaje.
* `COMPLETADA`: Proceso simulado finalizado con éxito.
* `ERROR`: Falla simulada por interrupción de red o espacio insuficiente.



### 11.5 Opción 5: Ver / gestionar historial de comandos

Proporciona visibilidad sobre la actividad desempeñada durante la sesión.

* **Información almacenada:** Marca de tiempo (`YYYY-MM-DD HH:MM:SS`), orden ingresada, tiempo de ejecución en milisegundos y código de retorno (`exit status`).
* **Acciones permitidas:**
1. Consultar el historial en pantalla.
2. Exportar el registro a un archivo de texto en disco.
3. Vaciar el historial de la memoria RAM.



### 11.6 Opción 0: Salir del sistema

Realiza una clausura limpia del programa:

* Notifica y detiene los hilos de descargas activos (`pthread_join`).
* Persiste el historial de comandos si está configurado.
* Libera los bloques de memoria dinámica asignados (`free`).
* Destruye los cerrojos y variables de condición.

---

## 12. Arquitectura general del proyecto

El diseño modular de **Admin_Sys** garantiza que la lógica de la interfaz principal esté desvinculada de los detalles de implementación técnica de cada herramienta:

```text
                   ┌──────────────┐
                   │    main.c    │
                   └──────┬───────┘
                          │
  ┌───────────┬───────────┼───────────┬───────────┐
  ▼           ▼           ▼           ▼           ▼
comandos.c respaldos.c analizador.c descargas.c procesos.c
  │           │           │           │           │
  └───────────┴───────────┼───────────┴───────────┘
                          ▼
                     archivos.c / utils.c

```

---

## 13. Descripción de los módulos principales

| Módulo | Archivos | Responsabilidad Técnica |
| --- | --- | --- |
| **Principal** | `main.c` | Despliega el menú, gestiona la navegación y controla el ciclo de vida global. |
| **Comandos** | `comandos.c`, `comandos.h` | Administra la creación de subprocesos mediante `fork()`, `execvp()` y redirección con `dup2()`. |
| **Respaldos** | `respaldos.c`, `respaldos.h` | Recorre directorios (`opendir`, `readdir`), evalúa metadatos (`stat`) y realiza copias de archivos. |
| **Analizador Bash** | `analizador_bash.c`, `analizador_bash.h` | Lee archivos de texto línea por línea y aplica análisis léxico para extraer métricas e identificar riesgos. |
| **Descargas** | `descargas.c`, `descargas.h` | Administra hilos POSIX (`pthread`), la cola de tareas compartida y la sincronización con mutex. |
| **Procesos** | `procesos.c`, `procesos.h` | Consulta y procesa estadísticas sobre tareas del sistema. |
| **Archivos** | `archivos.c`, `archivos.h` | Proporciona funciones de abstracción de bajo nivel para copiar, mover o borrar bloques de datos. |
| **Utilidades** | `utils.c`, `utils.h` | Ofrece funciones auxiliares para formatear cadenas, fechas y manejar la limpieza de pantalla. |

---

## 14. Ejemplos de uso real

### Ejemplo 1: Redirección de comando a archivo

1. Seleccione la opción `1`.
2. Escriba: `df -h > estado_discos.txt`
3. El archivo `estado_discos.txt` contendrá la información del espacio en disco del sistema.

### Ejemplo 2: Creación de respaldo incremental

1. Seleccione la opción `2`.
2. Ingrese la ruta origen: `/home/usuario/proyectos`
3. Ingrese la ruta destino: `/media/backup/proyectos`
4. El sistema analizará los cambios e informará cuántos archivos fueron copiados.

### Ejemplo 3: Análisis de script Bash

1. Seleccione la opción `3`.
2. Ingrese la ruta: `./scripts/instalar.sh`
3. Revise el reporte estructurado con métricas y alertas.

---

## 15. Ventajas del proyecto

* **Centralización:** Reúne múltiples herramientas de utilidad técnica en una sola terminal.
* **Diseño modular:** Estructura limpia que facilita la lectura, el mantenimiento y la expansión del código.
* **Eficiencia:** Desarrollado en C puro, garantizando un consumo mínimo de recursos de CPU y RAM.
* **Valor educativo:** Sirve como modelo práctico de llamadas al sistema POSIX, gestión de memoria, procesos e hilos.

---

## 16. Limitaciones del proyecto

* **Entorno simulado en descargas:** El gestor de descargas no realiza peticiones HTTP/FTP reales; imita el proceso para demostrar concurrencia.
* **Portabilidad:** Diseñado primordialmente para entornos compatibles con POSIX (Linux/UNIX/macOS). En Windows requiere capas de compatibilidad como WSL o MinGW.
* **Sintaxis de redirección simplificada:** El parser de comandos maneja redirecciones simples (`>` y `>>`), pero no cadenas complejas con múltiples tuberías (`pipes`).

---

## 17. Recomendaciones para el uso

1. **Permisos adecuados:** Verifique que el usuario con el que ejecuta el programa tenga permisos de lectura y escritura sobre los directorios seleccionados para respaldos o ejecuciones.
2. **Rutas absolutas o relativas válidas:** Al ingresar rutas de scripts o carpetas, asegúrese de escribir nombres y ubicaciones correctas.
3. **Recompilación tras cambios:** Si realiza variaciones en el código fuente dentro de la carpeta `src/`, recuerde ejecutar `make clean && make` antes de iniciar el sistema.

---

## 18. Conclusión

**Admin_Sys** combina simplicidad de uso y robustez técnica en una suite de administración CLI. Su arquitectura basada en módulos, el correcto uso de llamadas POSIX y el manejo de hilos la convierten en una solución didáctica, completa y representativa del desarrollo de software de bajo nivel en C.
