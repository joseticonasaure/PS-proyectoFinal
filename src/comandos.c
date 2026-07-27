#include "comandos.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <time.h>
#include <errno.h>

/* Variables globales de ambito de archivo para mantener la memoria del historial */
static EntradaHistorial *g_historial = NULL;
static size_t g_num_historial = 0;
static size_t g_capacidad_historial = 0;

/**
 * Auxiliar: Convierte una cadena de comando en un arreglo de argumentos de tipo argv (char**).
 */
static char **tokenizar_comando(const char *linea_comando, int *argc) {
    if (linea_comando == NULL) {
        return NULL;
    }

    char *copia = strdup(linea_comando);
    if (!copia) {
        perror("Error duplicando cadena de comando");
        return NULL;
    }

    int capacidad = 8;
    int contador = 0;
    char **argv = malloc(capacidad * sizeof(char *));
    if (!argv) {
        perror("Error al asignar memoria para argv");
        free(copia);
        return NULL;
    }

    char *token = strtok(copia, " \t\r\n");
    while (token != NULL) {
        if (contador + 1 >= capacidad) {
            capacidad *= 2;
            char **temp = realloc(argv, capacidad * sizeof(char *));
            if (!temp) {
                perror("Error al reasignar memoria para argv");
                free(copia);
                for (int i = 0; i < contador; i++) {
                    free(argv[i]);
                }
                free(argv);
                return NULL;
            }
            argv = temp;
        }
        argv[contador++] = strdup(token);
        token = strtok(NULL, " \t\r\n");
    }

    argv[contador] = NULL;
    free(copia);

    if (argc) {
        *argc = contador;
    }

    return argv;
}

/**
 * Auxiliar: Libera la memoria asignada a un arreglo de argumentos argv.
 */
static void liberar_argv(char **argv) {
    if (!argv) {
        return;
    }
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    free(argv);
}

int agregar_a_historial(const char *comando, int codigo_salida, double tiempo_ejecucion) {
    if (!comando || strlen(comando) == 0) {
        return -1;
    }

    if (g_num_historial >= g_capacidad_historial) {
        size_t nueva_cap = (g_capacidad_historial == 0) ? 16 : g_capacidad_historial * 2;
        EntradaHistorial *temp = realloc(g_historial, nueva_cap * sizeof(EntradaHistorial));
        if (!temp) {
            perror("Error al reasignar memoria de historial");
            return -1;
        }
        g_historial = temp;
        g_capacidad_historial = nueva_cap;
    }

    EntradaHistorial *e = &g_historial[g_num_historial];
    strncpy(e->comando, comando, sizeof(e->comando) - 1);
    e->comando[sizeof(e->comando) - 1] = '\0';
    e->fecha_hora = time(NULL);
    e->codigo_salida = codigo_salida;
    e->tiempo_ejecucion_seg = tiempo_ejecucion;

    g_num_historial++;
    return 0;
}

void mostrar_historial(void) {
    if (g_num_historial == 0) {
        imprimir_mensaje("El historial de comandos esta vacio.", COLOR_AMARILLO);
        return;
    }

    printf("%s%-5s %-20s %-8s %-12s %s%s\n",
           COLOR_NEGRITA, "#", "FECHA / HORA", "ESTADO", "TIEMPO (s)", "COMANDO", COLOR_RESET);
    printf("--------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < g_num_historial; i++) {
        EntradaHistorial *e = &g_historial[i];
        char fecha_str[20];
        struct tm *tm_info = localtime(&e->fecha_hora);
        if (tm_info) {
            strftime(fecha_str, sizeof(fecha_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            snprintf(fecha_str, sizeof(fecha_str), "Desconocida");
        }

        const char *color_estado = (e->codigo_salida == 0) ? COLOR_VERDE : COLOR_ROJO;

        printf("%-5zu %-20s %s%-8d%s %-12.4f %s\n",
               i + 1,
               fecha_str,
               color_estado, e->codigo_salida, COLOR_RESET,
               e->tiempo_ejecucion_seg,
               e->comando);
    }
    printf("--------------------------------------------------------------------------------\n");
}

int guardar_historial_archivo(const char *ruta_archivo) {
    const char *ruta = (ruta_archivo && strlen(ruta_archivo) > 0) ? ruta_archivo : ARCHIVO_HISTORIAL_DEF;
    FILE *f = fopen(ruta, "w");
    if (!f) {
        perror("Error al abrir archivo para guardar historial");
        return -1;
    }

    for (size_t i = 0; i < g_num_historial; i++) {
        EntradaHistorial *e = &g_historial[i];
        fprintf(f, "%lld\t%d\t%.6f\t%s\n",
                (long long)e->fecha_hora,
                e->codigo_salida,
                e->tiempo_ejecucion_seg,
                e->comando);
    }

    fclose(f);
    printf("%sHistorial guardado exitosamente en '%s'.%s\n", COLOR_VERDE, ruta, COLOR_RESET);
    return 0;
}

int cargar_historial_archivo(const char *ruta_archivo) {
    const char *ruta = (ruta_archivo && strlen(ruta_archivo) > 0) ? ruta_archivo : ARCHIVO_HISTORIAL_DEF;
    FILE *f = fopen(ruta, "r");
    if (!f) {
        return -1;
    }

    char linea[1024];
    while (fgets(linea, sizeof(linea), f)) {
        long long timestamp;
        int status;
        double tiempo;
        char cmd[MAX_HISTORIAL_CMD];

        if (sscanf(linea, "%lld\t%d\t%lf\t%[^\n]", &timestamp, &status, &tiempo, cmd) == 4) {
            if (g_num_historial >= g_capacidad_historial) {
                size_t nueva_cap = (g_capacidad_historial == 0) ? 16 : g_capacidad_historial * 2;
                EntradaHistorial *temp = realloc(g_historial, nueva_cap * sizeof(EntradaHistorial));
                if (!temp) {
                    fclose(f);
                    return -1;
                }
                g_historial = temp;
                g_capacidad_historial = nueva_cap;
            }
            EntradaHistorial *e = &g_historial[g_num_historial++];
            e->fecha_hora = (time_t)timestamp;
            e->codigo_salida = status;
            e->tiempo_ejecucion_seg = tiempo;
            strncpy(e->comando, cmd, sizeof(e->comando) - 1);
            e->comando[sizeof(e->comando) - 1] = '\0';
        }
    }

    fclose(f);
    return 0;
}

void vaciar_historial(void) {
    if (g_historial) {
        free(g_historial);
        g_historial = NULL;
    }
    g_num_historial = 0;
    g_capacidad_historial = 0;
    imprimir_mensaje("Historial vaciado de memoria.", COLOR_VERDE);
}

int ejecutar_comando_externo(const char *linea_comando) {
    if (!linea_comando || strlen(linea_comando) == 0) {
        return -1;
    }

    int argc = 0;
    char **argv = tokenizar_comando(linea_comando, &argc);
    if (!argv || argc == 0) {
        liberar_argv(argv);
        return -1;
    }

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int codigo_salida = -1;

#ifdef _WIN32
    if (argc > 0) {
        char comando_completo[1024];
        comando_completo[0] = '\0';
        for (int i = 0; i < argc; i++) {
            strncat(comando_completo, argv[i], sizeof(comando_completo) - strlen(comando_completo) - 1);
            if (i + 1 < argc) {
                strncat(comando_completo, " ", sizeof(comando_completo) - strlen(comando_completo) - 1);
            }
        }
        int rc = system(comando_completo);
        codigo_salida = rc;
    }
#else
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork()");
        liberar_argv(argv);
        return -1;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        perror("Error al ejecutar comando (execvp)");
        liberar_argv(argv);
        _exit(127);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error en waitpid()");
        } else {
            if (WIFEXITED(status)) {
                codigo_salida = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                codigo_salida = 128 + WTERMSIG(status);
            }
        }
    }
#endif

    clock_gettime(CLOCK_MONOTONIC, &fin);
    double tiempo_ejecucion = (double)(fin.tv_sec - inicio.tv_sec) + 
                              (double)(fin.tv_nsec - inicio.tv_nsec) / 1e9;

    liberar_argv(argv);
    agregar_a_historial(linea_comando, codigo_salida, tiempo_ejecucion);

    return codigo_salida;
}

int ejecutar_comando_redireccionado(const char *linea_comando, const char *archivo_salida, int es_append) {
    if (!linea_comando || !archivo_salida || strlen(linea_comando) == 0 || strlen(archivo_salida) == 0) {
        return -1;
    }

    int argc = 0;
    char **argv = tokenizar_comando(linea_comando, &argc);
    if (!argv || argc == 0) {
        liberar_argv(argv);
        return -1;
    }

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int codigo_salida = -1;

#ifdef _WIN32
    if (argc > 0) {
        char comando_completo[1024];
        comando_completo[0] = '\0';
        for (int i = 0; i < argc; i++) {
            strncat(comando_completo, argv[i], sizeof(comando_completo) - strlen(comando_completo) - 1);
            if (i + 1 < argc) {
                strncat(comando_completo, " ", sizeof(comando_completo) - strlen(comando_completo) - 1);
            }
        }
        char salida[1024];
        snprintf(salida, sizeof(salida), "%s %s %s", comando_completo, es_append ? ">>" : ">", archivo_salida);
        codigo_salida = system(salida);
    }
#else
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork()");
        liberar_argv(argv);
        return -1;
    }

    if (pid == 0) {
        int flags = O_WRONLY | O_CREAT | (es_append ? O_APPEND : O_TRUNC);
        int fd = open(archivo_salida, flags, 0644);
        if (fd < 0) {
            perror("Error al abrir archivo para redireccion");
            liberar_argv(argv);
            _exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Error en dup2()");
            close(fd);
            liberar_argv(argv);
            _exit(EXIT_FAILURE);
        }
        close(fd);

        execvp(argv[0], argv);
        perror("Error en execvp()");
        liberar_argv(argv);
        _exit(127);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Error en waitpid()");
        } else {
            if (WIFEXITED(status)) {
                codigo_salida = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                codigo_salida = 128 + WTERMSIG(status);
            }
        }
    }
#endif

    clock_gettime(CLOCK_MONOTONIC, &fin);
    double tiempo_ejecucion = (double)(fin.tv_sec - inicio.tv_sec) + 
                              (double)(fin.tv_nsec - inicio.tv_nsec) / 1e9;

    liberar_argv(argv);

    char cmd_con_redir[MAX_HISTORIAL_CMD];
    snprintf(cmd_con_redir, sizeof(cmd_con_redir), "%s %s %s",
             linea_comando, es_append ? ">>" : ">", archivo_salida);
    agregar_a_historial(cmd_con_redir, codigo_salida, tiempo_ejecucion);

    return codigo_salida;
}