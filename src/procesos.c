#include "procesos.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pwd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#ifdef _WIN32
typedef unsigned int uid_t;
#endif

/**
 * Auxiliar: Verifica si una cadena de texto contiene unicamente digitos numericos.
 */
static int es_numero(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

/**
 * Auxiliar: Traduce un UID numerico al nombre de usuario correspondiente del sistema.
 */
static void obtener_nombre_usuario(uid_t uid, char *buffer, size_t size) {
#ifdef _WIN32
    const char *usuario = getenv("USERNAME");
    if (usuario && *usuario) {
        snprintf(buffer, size, "%s", usuario);
    } else {
        snprintf(buffer, size, "%u", (unsigned int)uid);
    }
#else
    struct passwd *pw = getpwuid(uid);
    if (pw && pw->pw_name) {
        snprintf(buffer, size, "%s", pw->pw_name);
    } else {
        snprintf(buffer, size, "%u", (unsigned int)uid);
    }
#endif
}

/**
 * Auxiliar: Parsea el archivo /proc/[PID]/stat para extraer métricas del proceso.
 */
static int parsear_proc_stat(pid_t pid, ProcesoInfo *info) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/proc/%d/stat", pid);

    FILE *f = fopen(filepath, "r");
    if (!f) {
        return -1;
    }

    char buffer[2048];
    if (!fgets(buffer, sizeof(buffer), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    /* El campo del nombre ejecutable esta delimitado entre '(' y ')' */
    char *open_paren = strchr(buffer, '(');
    char *close_paren = strrchr(buffer, ')');
    if (!open_paren || !close_paren || close_paren <= open_paren) {
        return -1;
    }

    info->pid = pid;

    /* Extraer nombre del ejecutable */
    size_t len_nombre = (size_t)(close_paren - open_paren - 1);
    if (len_nombre >= sizeof(info->nombre)) {
        len_nombre = sizeof(info->nombre) - 1;
    }
    strncpy(info->nombre, open_paren + 1, len_nombre);
    info->nombre[len_nombre] = '\0';

    /* Parsear variables despues del ultimo par de parentesis */
    char estado;
    int ppid;
    unsigned long utime = 0, stime = 0, vsize = 0;
    long rss = 0;

    int parseados = sscanf(close_paren + 2,
                           "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %ld",
                           &estado, &ppid, &utime, &stime, &vsize, &rss);

    if (parseados < 6) {
        return -1;
    }

    info->estado = estado;
    info->ppid = (pid_t)ppid;
    info->utime = utime;
    info->stime = stime;
    info->vsize_kb = vsize / 1024;

    /* Convertir paginas de memoria de RSS a KBytes */
#ifdef _WIN32
    info->memoria_rss_kb = (unsigned long)(rss * 4);
#else
    long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    info->memoria_rss_kb = (unsigned long)(rss * page_size_kb);
#endif

    /* Obtener propietario leyendo /proc/[PID]/status */
    snprintf(filepath, sizeof(filepath), "/proc/%d/status", pid);
    FILE *fs = fopen(filepath, "r");
    if (fs) {
        char line[256];
        uid_t uid = 0;
        while (fgets(line, sizeof(line), fs)) {
            if (strncmp(line, "Uid:", 4) == 0) {
                sscanf(line + 4, "%u", &uid);
                break;
            }
        }
        fclose(fs);
        obtener_nombre_usuario(uid, info->usuario, sizeof(info->usuario));
    } else {
        snprintf(info->usuario, sizeof(info->usuario), "desconocido");
    }

    return 0;
}

int obtener_info_proceso(pid_t pid, ProcesoInfo *info) {
    if (info == NULL || pid <= 0) {
        return -1;
    }
    return parsear_proc_stat(pid, info);
}

int listar_procesos(ProcesoInfo **lista, size_t *num_procesos) {
    if (lista == NULL || num_procesos == NULL) {
        return -1;
    }

    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Error al abrir /proc");
        return -1;
    }

    size_t capacidad = 128;
    size_t cantidad = 0;
    ProcesoInfo *arreglo = malloc(capacidad * sizeof(ProcesoInfo));
    if (!arreglo) {
        perror("Error de asignacion de memoria");
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (es_numero(entry->d_name)) {
            pid_t pid = (pid_t)atoi(entry->d_name);
            ProcesoInfo info;

            if (obtener_info_proceso(pid, &info) == 0) {
                if (cantidad >= capacidad) {
                    capacidad *= 2;
                    ProcesoInfo *temp = realloc(arreglo, capacidad * sizeof(ProcesoInfo));
                    if (!temp) {
                        perror("Error al reasignar memoria para procesos");
                        free(arreglo);
                        closedir(dir);
                        return -1;
                    }
                    arreglo = temp;
                }
                arreglo[cantidad++] = info;
            }
        }
    }

    closedir(dir);
    *lista = arreglo;
    *num_procesos = cantidad;
    return 0;
}

void liberar_lista_procesos(ProcesoInfo *lista) {
    if (lista != NULL) {
        free(lista);
    }
}

int enviar_senal_proceso(pid_t pid, int senal) {
    if (pid <= 0) {
        fprintf(stderr, "PID invalido: %d\n", pid);
        return -1;
    }

#ifdef _WIN32
    (void)pid;
    (void)senal;
    fprintf(stderr, "Envio de senales no soportado en esta plataforma.\n");
    return -1;
#else
    if (kill(pid, senal) == -1) {
        perror("Error al enviar la senal con kill()");
        return -1;
    }

    return 0;
#endif
}

void mostrar_tabla_procesos(void) {
    ProcesoInfo *lista = NULL;
    size_t total = 0;

    if (listar_procesos(&lista, &total) != 0) {
        imprimir_mensaje("Error al obtener la lista de procesos.", COLOR_ROJO);
        return;
    }

    printf("%s%-8s %-8s %-16s %-8s %-12s %-25s%s\n",
           COLOR_NEGRITA, "PID", "PPID", "USUARIO", "ESTADO", "RSS (KB)", "COMANDO", COLOR_RESET);
    printf("-------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < total; i++) {
        const char *color_estado = COLOR_RESET;
        switch (lista[i].estado) {
            case 'R': color_estado = COLOR_VERDE; break;     /* Ejecutando */
            case 'S': color_estado = COLOR_AZUL; break;      /* Durmiendo interruptible */
            case 'D': color_estado = COLOR_AMARILLO; break;  /* Espera E/S no interruptible */
            case 'Z': color_estado = COLOR_ROJO; break;      /* Zombi */
            case 'T': color_estado = COLOR_MAGENTA; break;   /* Detenido */
            default: color_estado = COLOR_RESET; break;
        }

        printf("%-8d %-8d %-16s %s%-8c%s %-12lu %-25s\n",
               lista[i].pid,
               lista[i].ppid,
               lista[i].usuario,
               color_estado, lista[i].estado, COLOR_RESET,
               lista[i].memoria_rss_kb,
               lista[i].nombre);
    }

    printf("-------------------------------------------------------------------------------\n");
    printf("%sTotal de procesos detectados: %size_t %zu%s\n", COLOR_CYAN, COLOR_NEGRITA, total, COLOR_RESET);

    liberar_lista_procesos(lista);
}

/**
 * Auxiliar recursivo para imprimir la jerarquia del arbol de procesos.
 */
static void imprimir_nodo_arbol(const ProcesoInfo *lista, size_t total, pid_t ppid_actual, int nivel) {
    for (size_t i = 0; i < total; i++) {
        if (lista[i].ppid == ppid_actual) {
            for (int k = 0; k < nivel; k++) {
                printf("  │ ");
            }
            printf("  ├── [%s%d%s] %s (%sKB)\n",
                   COLOR_CYAN, lista[i].pid, COLOR_RESET,
                   lista[i].nombre,
                   COLOR_AMARILLO);

            /* Llamada recursiva para procesar los procesos hijos */
            imprimir_nodo_arbol(lista, total, lista[i].pid, nivel + 1);
        }
    }
}

void mostrar_arbol_procesos(void) {
    ProcesoInfo *lista = NULL;
    size_t total = 0;

    if (listar_procesos(&lista, &total) != 0) {
        imprimir_mensaje("Error al cargar procesos para el arbol.", COLOR_ROJO);
        return;
    }

    imprimir_mensaje("Jerarquia de Procesos (Árbol PPID -> PID):", COLOR_NEGRITA);
    /* Iniciar desde PID 0 / PID 1 (systemd/init) */
    imprimir_nodo_arbol(lista, total, 0, 0);
    imprimir_nodo_arbol(lista, total, 1, 0);

    liberar_lista_procesos(lista);
}

void mostrar_detalles_proceso(pid_t pid) {
    ProcesoInfo info;
    if (obtener_info_proceso(pid, &info) != 0) {
        printf("%sError: No se encontro informacion para el PID %d.%s\n", COLOR_ROJO, pid, COLOR_RESET);
        return;
    }

    printf("%s=== Detalles del Proceso PID %d ===%s\n", COLOR_CYAN, pid, COLOR_RESET);
    printf("  Nombre Ejecutable : %s\n", info.nombre);
    printf("  PID               : %d\n", info.pid);
    printf("  PPID (Padre)      : %d\n", info.ppid);
    printf("  Usuario           : %s\n", info.usuario);
    printf("  Estado            : %c\n", info.estado);
    printf("  Memoria RSS       : %lu KB\n", info.memoria_rss_kb);
    printf("  Memoria Virtual   : %lu KB\n", info.vsize_kb);
    printf("  Tiempo CPU (User) : %lu jiffies\n", info.utime);
    printf("  Tiempo CPU (Sys)  : %lu jiffies\n", info.stime);

    /* Intentar leer la linea de comandos completa (/proc/[PID]/cmdline) */
    char cmdpath[512];
    snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/cmdline", pid);
    FILE *f = fopen(cmdpath, "r");
    if (f) {
        char cmdline[1024];
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
        fclose(f);
        if (n > 0) {
            cmdline[n] = '\0';
            /* Reemplazar los caracteres nulos internos por espacios */
            for (size_t i = 0; i < n - 1; i++) {
                if (cmdline[i] == '\0') {
                    cmdline[i] = ' ';
                }
            }
            printf("  Comando completo  : %s\n", cmdline);
        } else {
            printf("  Comando completo  : [%s]\n", info.nombre);
        }
    }
    printf("========================================\n");
}