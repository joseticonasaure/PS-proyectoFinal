#ifndef PROCESOS_H
#define PROCESOS_H

#include <sys/types.h>
#include <stddef.h>

#define MAX_NOMBRE_PROC 256
#define MAX_USUARIO_PROC 64

/**
 * Estructura para almacenar las metricas y metadatos de un proceso leido de /proc.
 */
typedef struct {
    pid_t pid;                           /* Identificador del proceso */
    pid_t ppid;                          /* Identificador del proceso padre */
    char nombre[MAX_NOMBRE_PROC];        /* Nombre ejecutable del proceso */
    char estado;                         /* Estado: R, S, D, Z, T, etc. */
    unsigned long memoria_rss_kb;        /* Memoria residente en KBytes */
    unsigned long vsize_kb;              /* Memoria virtual total en KBytes */
    char usuario[MAX_USUARIO_PROC];      /* Nombre del usuario propietario */
    unsigned long utime;                 /* Tiempo de CPU en modo usuario (jiffies) */
    unsigned long stime;                 /* Tiempo de CPU en modo kernel (jiffies) */
} ProcesoInfo;

/**
 * Obtiene la lista completa de procesos en ejecucion escaneando el directorio /proc.
 * @param lista Puntero donde se asignara la memoria con el arreglo de procesos.
 * @param num_procesos Puntero donde se guardara la cantidad de procesos encontrados.
 * @return 0 en exito, -1 en caso de error.
 */
int listar_procesos(ProcesoInfo **lista, size_t *num_procesos);

/**
 * Libera la memoria asignada dinamicamente para la lista de procesos.
 * @param lista Arreglo de estructuras ProcesoInfo a liberar.
 */
void liberar_lista_procesos(ProcesoInfo *lista);

/**
 * Obtiene la informacion detallada de un unico proceso por su PID.
 * @param pid Identificador del proceso a consultar.
 * @param info Puntero a la estructura donde se cargaran los datos.
 * @return 0 en exito, -1 si el proceso no existe o falla la lectura.
 */
int obtener_info_proceso(pid_t pid, ProcesoInfo *info);

/**
 * Envia una senal POSIX a un proceso especifico usando kill.
 * @param pid PID del proceso destino.
 * @param senal Numero de la senal (ej: SIGKILL, SIGTERM, SIGSTOP).
 * @return 0 en exito, -1 en caso de error.
 */
int enviar_senal_proceso(pid_t pid, int senal);

/**
 * Muestra en la terminal una tabla con formato interactivo de todos los procesos.
 */
void mostrar_tabla_procesos(void);

/**
 * Muestra la jerarquia de procesos en forma de arbol dependiente (PID vs PPID).
 */
void mostrar_arbol_procesos(void);

/**
 * Muestra en detalle la informacion, linea de comandos y metricas de un PID dado.
 * @param pid PID del proceso a consultar.
 */
void mostrar_detalles_proceso(pid_t pid);

#endif /* PROCESOS_H */