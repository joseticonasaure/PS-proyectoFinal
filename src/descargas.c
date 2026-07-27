#define _POSIX_C_SOURCE 200809L

#include "descargas.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Instancia global estatica de la cola de descargas */
static ColaDescargas g_cola;
static bool g_cola_inicializada = false;

/**
 * Auxiliar: Convierte el enum de EstadoDescarga a una cadena de texto para presentacion.
 */
static const char *obtener_texto_estado(EstadoDescarga estado) {
    switch (estado) {
        case ESTADO_PENDIENTE:   return "PENDIENTE";
        case ESTADO_EN_PROGRESO: return "DESCARGANDO";
        case ESTADO_COMPLETADO:  return "COMPLETADO";
        case ESTADO_ERROR:       return "ERROR";
        case ESTADO_CANCELADO:   return "CANCELADO";
        default:                 return "DESCONOCIDO";
    }
}

/**
 * Funcion ejecutada por cada hilo del pool POSIX.
 */
static void *trabajador_descargas(void *arg) {
    (void)arg;

    while (1) {
        TareaDescarga *tarea_actual = NULL;

        pthread_mutex_lock(&g_cola.lock);

        /* Esperar por trabajo mientras la cola este activa y no haya tareas pendientes */
        while (g_cola.ejecutando) {
            for (size_t i = 0; i < g_cola.total_tareas; i++) {
                if (g_cola.tareas[i].estado == ESTADO_PENDIENTE) {
                    tarea_actual = &g_cola.tareas[i];
                    tarea_actual->estado = ESTADO_EN_PROGRESO;
                    break;
                }
            }

            if (tarea_actual != NULL) {
                break; /* Tarea encontrada */
            }

            /* Liberar mutex y suspender hilo hasta nueva notificacion */
            pthread_cond_wait(&g_cola.cond_trabajo, &g_cola.lock);
        }

        /* Si la cola fue apagada y no hay tareas en ejecucion, terminar hilo */
        if (!g_cola.ejecutando && tarea_actual == NULL) {
            pthread_mutex_unlock(&g_cola.lock);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&g_cola.lock);

        if (tarea_actual != NULL) {
            /* Simulación de descarga fragmentada por bloques */
            size_t bloque = 1024 * 64; /* Bloques de 64 KB */
            struct timespec inicio, fin;

            while (tarea_actual->bytes_descargados < tarea_actual->tamano_bytes) {
                clock_gettime(CLOCK_MONOTONIC, &inicio);

                /* Pausa controlada para simular latencia de red (50 ms) */
                usleep(50000);

                pthread_mutex_lock(&g_cola.lock);
                tarea_actual->bytes_descargados += bloque;
                if (tarea_actual->bytes_descargados > tarea_actual->tamano_bytes) {
                    tarea_actual->bytes_descargados = tarea_actual->tamano_bytes;
                }

                clock_gettime(CLOCK_MONOTONIC, &fin);
                double tiempo_s = (double)(fin.tv_sec - inicio.tv_sec) +
                                  (double)(fin.tv_nsec - inicio.tv_nsec) / 1e9;

                if (tiempo_s > 0) {
                    tarea_actual->velocidad_kbps = ((double)bloque / 1024.0) / tiempo_s;
                }

                pthread_mutex_unlock(&g_cola.lock);
            }

            pthread_mutex_lock(&g_cola.lock);
            tarea_actual->estado = ESTADO_COMPLETADO;
            tarea_actual->velocidad_kbps = 0.0;
            pthread_mutex_unlock(&g_cola.lock);
        }
    }

    return NULL;
}

int inicializar_cola_descargas(int max_hilos) {
    if (g_cola_inicializada) {
        return 0;
    }

    if (max_hilos <= 0) {
        max_hilos = 2;
    }

    memset(&g_cola, 0, sizeof(ColaDescargas));
    g_cola.max_hilos = max_hilos;
    g_cola.capacidad = 8;
    g_cola.contador_id = 1;
    g_cola.ejecutando = true;

    g_cola.tareas = malloc(g_cola.capacidad * sizeof(TareaDescarga));
    if (!g_cola.tareas) {
        perror("Error de asignacion para tareas de descarga");
        return -1;
    }

    g_cola.hilos_trabajadores = malloc(max_hilos * sizeof(pthread_t));
    if (!g_cola.hilos_trabajadores) {
        perror("Error de asignacion para hilos trabajadores");
        free(g_cola.tareas);
        return -1;
    }

    if (pthread_mutex_init(&g_cola.lock, NULL) != 0) {
        perror("Error en pthread_mutex_init");
        free(g_cola.tareas);
        free(g_cola.hilos_trabajadores);
        return -1;
    }

    if (pthread_cond_init(&g_cola.cond_trabajo, NULL) != 0) {
        perror("Error en pthread_cond_init");
        pthread_mutex_destroy(&g_cola.lock);
        free(g_cola.tareas);
        free(g_cola.hilos_trabajadores);
        return -1;
    }

    /* Crear los hilos trabajadores del pool */
    for (int i = 0; i < max_hilos; i++) {
        if (pthread_create(&g_cola.hilos_trabajadores[i], NULL, trabajador_descargas, NULL) != 0) {
            perror("Error al crear hilo trabajador");
            detener_cola_descargas();
            destruir_cola_descargas();
            return -1;
        }
    }

    g_cola_inicializada = true;
    return 0;
}

int agregar_tarea_descarga(const char *origen, const char *destino, size_t tamano_simulado_bytes) {
    if (!g_cola_inicializada) {
        if (inicializar_cola_descargas(2) != 0) {
            return -1;
        }
    }

    if (!origen || !destino || strlen(origen) == 0 || strlen(destino) == 0) {
        return -1;
    }

    pthread_mutex_lock(&g_cola.lock);

    if (g_cola.total_tareas >= g_cola.capacidad) {
        size_t nueva_cap = g_cola.capacidad * 2;
        TareaDescarga *temp = realloc(g_cola.tareas, nueva_cap * sizeof(TareaDescarga));
        if (!temp) {
            perror("Error reasignando memoria para tareas de descarga");
            pthread_mutex_unlock(&g_cola.lock);
            return -1;
        }
        g_cola.tareas = temp;
        g_cola.capacidad = nueva_cap;
    }

    TareaDescarga *t = &g_cola.tareas[g_cola.total_tareas++];
    t->id = g_cola.contador_id++;
    strncpy(t->origen, origen, sizeof(t->origen) - 1);
    t->origen[sizeof(t->origen) - 1] = '\0';
    strncpy(t->destino, destino, sizeof(t->destino) - 1);
    t->destino[sizeof(t->destino) - 1] = '\0';
    t->tamano_bytes = (tamano_simulado_bytes > 0) ? tamano_simulado_bytes : (1024 * 1024 * 5); /* 5 MB def */
    t->bytes_descargados = 0;
    t->estado = ESTADO_PENDIENTE;
    t->velocidad_kbps = 0.0;

    int tarea_id = t->id;

    /* Notificar a un hilo disponible sobre la nueva tarea */
    pthread_cond_signal(&g_cola.cond_trabajo);
    pthread_mutex_unlock(&g_cola.lock);

    return tarea_id;
}

void mostrar_estado_cola(void) {
    if (!g_cola_inicializada) {
        imprimir_mensaje("La cola de descargas no esta inicializada.", COLOR_AMARILLO);
        return;
    }

    pthread_mutex_lock(&g_cola.lock);

    if (g_cola.total_tareas == 0) {
        imprimir_mensaje("No hay tareas registradas en la cola de descargas.", COLOR_AMARILLO);
        pthread_mutex_unlock(&g_cola.lock);
        return;
    }

    printf("%s%-4s %-20s %-12s %-10s %-12s %s%s\n",
           COLOR_NEGRITA, "ID", "ORIGEN", "ESTADO", "PROGRESO", "VELOCIDAD", "DESTINO", COLOR_RESET);
    printf("--------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < g_cola.total_tareas; i++) {
        TareaDescarga *t = &g_cola.tareas[i];

        float porcentaje = 0.0f;
        if (t->tamano_bytes > 0) {
            porcentaje = ((float)t->bytes_descargados / (float)t->tamano_bytes) * 100.0f;
        }

        const char *color_estado = COLOR_RESET;
        switch (t->estado) {
            case ESTADO_COMPLETADO:  color_estado = COLOR_VERDE; break;
            case ESTADO_EN_PROGRESO: color_estado = COLOR_CYAN; break;
            case ESTADO_PENDIENTE:   color_estado = COLOR_AMARILLO; break;
            case ESTADO_ERROR:       color_estado = COLOR_ROJO; break;
            default:                 color_estado = COLOR_RESET; break;
        }

        /* Recortar cadenas si superan el ancho de columna */
        char origen_corto[21];
        strncpy(origen_corto, t->origen, 20);
        origen_corto[20] = '\0';

        char destino_corto[21];
        strncpy(destino_corto, t->destino, 20);
        destino_corto[20] = '\0';

        printf("%-4d %-20s %s%-12s%s %-9.1f%% %-9.1f KB/s %s\n",
               t->id,
               origen_corto,
               color_estado, obtener_texto_estado(t->estado), COLOR_RESET,
               porcentaje,
               t->velocidad_kbps,
               destino_corto);
    }

    printf("--------------------------------------------------------------------------------\n");
    pthread_mutex_unlock(&g_cola.lock);
}

void detener_cola_descargas(void) {
    if (!g_cola_inicializada) {
        return;
    }

    pthread_mutex_lock(&g_cola.lock);
    g_cola.ejecutando = false;
    /* Notificar a todos los hilos suspendidos para que salgan del bucle */
    pthread_cond_broadcast(&g_cola.cond_trabajo);
    pthread_mutex_unlock(&g_cola.lock);

    /* Esperar la finalización segura de todos los hilos */
    for (int i = 0; i < g_cola.max_hilos; i++) {
        pthread_join(g_cola.hilos_trabajadores[i], NULL);
    }
}

void destruir_cola_descargas(void) {
    if (!g_cola_inicializada) {
        return;
    }

    detener_cola_descargas();

    pthread_mutex_destroy(&g_cola.lock);
    pthread_cond_destroy(&g_cola.cond_trabajo);

    if (g_cola.tareas) {
        free(g_cola.tareas);
        g_cola.tareas = NULL;
    }

    if (g_cola.hilos_trabajadores) {
        free(g_cola.hilos_trabajadores);
        g_cola.hilos_trabajadores = NULL;
    }

    g_cola_inicializada = false;
}