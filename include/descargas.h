#ifndef DESCARGAS_H
#define DESCARGAS_H

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_URL_LEN 512
#define MAX_PATH_LEN 512

/**
 * Estados posibles de una tarea de descarga dentro de la cola.
 */
typedef enum {
    ESTADO_PENDIENTE,
    ESTADO_EN_PROGRESO,
    ESTADO_COMPLETADO,
    ESTADO_ERROR,
    ESTADO_CANCELADO
} EstadoDescarga;

/**
 * Estructura para representar una tarea de descarga individual.
 */
typedef struct {
    int id;                           /* Identificador unico de la tarea */
    char origen[MAX_URL_LEN];         /* URL o ruta origen del recurso */
    char destino[MAX_PATH_LEN];       /* Ruta local del archivo de destino */
    size_t tamano_bytes;              /* Tamano total del archivo a descargar */
    size_t bytes_descargados;         /* Bytes efectivamente transferidos */
    EstadoDescarga estado;            /* Estado actual del proceso */
    double velocidad_kbps;            /* Velocidad estimada de descarga en KB/s */
} TareaDescarga;

/**
 * Estructura principal que gestiona la cola dinamica y el pool de hilos POSIX.
 */
typedef struct {
    TareaDescarga *tareas;            /* Arreglo dinamico de tareas */
    size_t total_tareas;              /* Contador de tareas ingresadas */
    size_t capacidad;                 /* Capacidad maxima del arreglo dinamico */
    int max_hilos;                    /* Numero de hilos concurrentes activos */
    pthread_t *hilos_trabajadores;    /* Identificadores de hilos POSIX */
    pthread_mutex_t lock;             /* Mutex para sincronizacion de memoria compartida */
    pthread_cond_t cond_trabajo;      /* Variable de condicion para la cola de tareas */
    bool ejecutando;                  /* Bandera para apagar el pool de hilos de forma segura */
    int contador_id;                  /* Generador de IDs secuenciales */
} ColaDescargas;

/**
 * Inicializa la cola de descargas y crea el pool de hilos trabajadores.
 * @param max_hilos Numero maximo de descargas simultaneas/concurrentes.
 * @return 0 en exito, -1 en caso de error.
 */
int inicializar_cola_descargas(int max_hilos);

/**
 * Agrega una nueva tarea de descarga a la cola global.
 * @param origen URL o identificador del recurso origen.
 * @param destino Ruta local donde se guardara el archivo.
 * @param tamano_simulado_bytes Tamano en bytes a simular para la transferencia.
 * @return ID de la tarea creada (>0) o -1 en caso de error.
 */
int agregar_tarea_descarga(const char *origen, const char *destino, size_t tamano_simulado_bytes);

/**
 * Muestra en pantalla el estado actual de todas las tareas y sus barras de progreso.
 */
void mostrar_estado_cola(void);

/**
 * Notifica a los hilos trabajadores que no acepten mas tareas y detiene el pool.
 */
void detener_cola_descargas(void);

/**
 * Libera todos los recursos en memoria (mutex, variables de condicion, memoria dinamica).
 */
void destruir_cola_descargas(void);

#endif /* DESCARGAS_H */