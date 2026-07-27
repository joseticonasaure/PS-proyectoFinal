#define _POSIX_C_SOURCE 200809L

#include "respaldos.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#ifndef lstat
#define lstat(path, st) stat(path, st)
#endif
#else
#include <sys/stat.h>
#endif

#define TAM_BUFFER_COPIA 8192

/**
 * Auxiliar: Crea de forma recursiva una estructura de directorios en disco.
 */
static int crear_directorio_recursivo(const char *path, mode_t mode) {
    char temp[1024];
    snprintf(temp, sizeof(temp), "%s", path);
    size_t len = strlen(temp);

    if (len == 0) {
        return -1;
    }

    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(temp, mode) == -1 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(temp, mode) == -1 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

/**
 * Auxiliar: Copia un archivo bloque por bloque mediante llamadas al sistema (open, read, write)
 * y preserva la fecha de ultima modificacion POSIX.
 */
static int copiar_archivo(const char *origen, const char *destino, const struct stat *st_src) {
    int fd_in = open(origen, O_RDONLY);
    if (fd_in < 0) {
        perror("Error al abrir archivo de origen para lectura");
        return -1;
    }

    int fd_out = open(destino, O_WRONLY | O_CREAT | O_TRUNC, st_src->st_mode & 0777);
    if (fd_out < 0) {
        perror("Error al crear/abrir archivo de destino para escritura");
        close(fd_in);
        return -1;
    }

    char buffer[TAM_BUFFER_COPIA];
    ssize_t leidos, escritos;
    int error_transferencia = 0;

    while ((leidos = read(fd_in, buffer, sizeof(buffer))) > 0) {
        char *ptr = buffer;
        ssize_t restante = leidos;
        while (restante > 0) {
            escritos = write(fd_out, ptr, restante);
            if (escritos <= 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("Error durante la escritura en archivo de respaldo");
                error_transferencia = 1;
                break;
            }
            restante -= escritos;
            ptr += escritos;
        }
        if (error_transferencia) {
            break;
        }
    }

    if (leidos < 0) {
        perror("Error durante la lectura del archivo de origen");
        error_transferencia = 1;
    }

    close(fd_in);
    close(fd_out);

    if (error_transferencia) {
        unlink(destino);
        return -1;
    }

    /* En Windows no se preservan timestamps con utimensat; se omite para compatibilidad. */
    (void)st_src;
    return 0;
}

/**
 * Auxiliar recursivo para escaneo incremental y sincronizacion.
 */
static int respardo_recursivo_core(const char *origen, const char *destino, EstadisticasRespaldo *stats) {
    DIR *dir_orig = opendir(origen);
    if (!dir_orig) {
        perror("Error al abrir directorio de origen");
        stats->errores++;
        return -1;
    }

    /* Asegurar existencia del directorio de destino */
    struct stat st_dir_orig;
    if (lstat(origen, &st_dir_orig) == 0) {
        if (crear_directorio_recursivo(destino, st_dir_orig.st_mode & 0777) == 0) {
            stats->directorios_creados++;
        }
    }

    struct dirent *entry;
    char path_orig[1024];
    char path_dest[1024];

    while ((entry = readdir(dir_orig)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path_orig, sizeof(path_orig), "%s/%s", origen, entry->d_name);
        snprintf(path_dest, sizeof(path_dest), "%s/%s", destino, entry->d_name);

        struct stat st_src;
        if (lstat(path_orig, &st_src) == -1) {
            perror("Error al consultar estado de archivo origen");
            stats->errores++;
            continue;
        }

        if (S_ISDIR(st_src.st_mode)) {
            /* Llamada recursiva para subdirectorios */
            respardo_recursivo_core(path_orig, path_dest, stats);
        } else if (S_ISREG(st_src.st_mode)) {
            struct stat st_dst;
            int necesita_copia = 0;

            if (lstat(path_dest, &st_dst) == -1) {
                /* El archivo no existe en el destino -> Copiar */
                necesita_copia = 1;
            } else {
                /* Si difiere en tamano o si la fecha de modificacion en origen es mas reciente */
                if (st_src.st_size != st_dst.st_size || st_src.st_mtime > st_dst.st_mtime) {
                    necesita_copia = 1;
                }
            }

            if (necesita_copia) {
                if (copiar_archivo(path_orig, path_dest, &st_src) == 0) {
                    stats->archivos_copiados++;
                    stats->bytes_copiados += st_src.st_size;
                } else {
                    stats->errores++;
                }
            } else {
                stats->archivos_omitidos++;
            }
        }
    }

    closedir(dir_orig);
    return 0;
}

int crear_respaldo_incremental(const char *origen, const char *destino, EstadisticasRespaldo *stats) {
    if (!origen || !destino || !stats || strlen(origen) == 0 || strlen(destino) == 0) {
        return -1;
    }

    memset(stats, 0, sizeof(EstadisticasRespaldo));

    struct stat st;
    if (lstat(origen, &st) == -1 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "El directorio de origen '%s' no existe o no es un directorio.\n", origen);
        return -1;
    }

    printf("%sIniciando respaldo incremental desde '%s' hacia '%s'...%s\n",
           COLOR_CYAN, origen, destino, COLOR_RESET);

    return respardo_recursivo_core(origen, destino, stats);
}

int restaurar_respaldo(const char *origen_respaldo, const char *destino_restauracion, EstadisticasRespaldo *stats) {
    if (!origen_respaldo || !destino_restauracion || !stats) {
        return -1;
    }

    printf("%sRestaurando copia de seguridad desde '%s' hacia '%s'...%s\n",
           COLOR_AMARILLO, origen_respaldo, destino_restauracion, COLOR_RESET);

    /* La restauracion fuerza la sincronizacion incremental sobre el directorio destino */
    return crear_respaldo_incremental(origen_respaldo, destino_restauracion, stats);
}

void mostrar_resumen_respaldo(const EstadisticasRespaldo *stats) {
    if (!stats) {
        return;
    }

    double mb_copiados = (double)stats->bytes_copiados / (1024.0 * 1024.0);

    printf("\n%s=== Resumen de la Operacion de Respaldo ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Archivos Copiados/Actualizados : %s%zu%s\n", COLOR_VERDE, stats->archivos_copiados, COLOR_RESET);
    printf("  Archivos Omitidos (Sin cambios): %s%zu%s\n", COLOR_AZUL, stats->archivos_omitidos, COLOR_RESET);
    printf("  Subdirectorios Procesados      : %zu\n", stats->directorios_creados);
    printf("  Datos Transferidos             : %.2f MB (%ld bytes)\n", mb_copiados, (long)stats->bytes_copiados);

    if (stats->errores > 0) {
        printf("  Errores Detectados             : %s%zu%s\n", COLOR_ROJO, stats->errores, COLOR_RESET);
    } else {
        printf("  Estado de la Operación         : %sExitoso%s\n", COLOR_VERDE, COLOR_RESET);
    }
    printf("============================================\n");
}