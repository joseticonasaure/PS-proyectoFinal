#include "archivos.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pwd.h>
#include <grp.h>
#endif
#include <time.h>
#include <errno.h>

#ifdef _WIN32
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#ifndef S_IFLNK
#define S_IFLNK 0xA000
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) 0
#endif
#ifndef lstat
#define lstat(path, st) stat(path, st)
#endif
#endif

/**
 * Auxiliar: Convierte el modo de proteccion en una cadena estilo 'ls -l' (ej: -rwxr-xr-x).
 */
static void formato_permisos_cadena(mode_t mode, char *str) {
    str[0] = S_ISDIR(mode)  ? 'd' :
             S_ISLNK(mode)  ? 'l' :
             S_ISCHR(mode)  ? 'c' :
             S_ISBLK(mode)  ? 'b' :
             S_ISFIFO(mode) ? 'p' :
             S_ISSOCK(mode) ? 's' : '-';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

/**
 * Auxiliar: Traduce UID y GID a nombres de usuario y grupo respetando buffers.
 */
static void obtener_propietario_y_grupo(uid_t uid, gid_t gid, char *usr_buf, size_t usr_size, char *grp_buf, size_t grp_size) {
#ifdef _WIN32
    snprintf(usr_buf, usr_size, "%u", (unsigned int)uid);
    snprintf(grp_buf, grp_size, "%u", (unsigned int)gid);
#else
    struct passwd *pw = getpwuid(uid);
    if (pw && pw->pw_name) {
        snprintf(usr_buf, usr_size, "%s", pw->pw_name);
    } else {
        snprintf(usr_buf, usr_size, "%u", (unsigned int)uid);
    }

    struct group *gr = getgrgid(gid);
    if (gr && gr->gr_name) {
        snprintf(grp_buf, grp_size, "%s", gr->gr_name);
    } else {
        snprintf(grp_buf, grp_size, "%u", (unsigned int)gid);
    }
#endif
}

int listar_directorio(const char *ruta) {
    const char *dir_ruta = (ruta != NULL && strlen(ruta) > 0) ? ruta : ".";

    DIR *dir = opendir(dir_ruta);
    if (!dir) {
        perror("Error al abrir el directorio");
        return -1;
    }

    printf("%sListando directorio: %s%s\n\n", COLOR_CYAN, dir_ruta, COLOR_RESET);
    printf("%s%-11s %-12s %-12s %-10s %-16s %s%s\n",
           COLOR_NEGRITA, "PERMISOS", "USUARIO", "GRUPO", "TAMANO", "MODIFICADO", "NOMBRE", COLOR_RESET);
    printf("--------------------------------------------------------------------------------\n");

    struct dirent *entry;
    char path_completo[1024];

    while ((entry = readdir(dir)) != NULL) {
        /* Construir ruta completa para lstat */
        snprintf(path_completo, sizeof(path_completo), "%s/%s", dir_ruta, entry->d_name);

        struct stat st;
        if (lstat(path_completo, &st) == -1) {
            continue;
        }

        char permisos[11];
        formato_permisos_cadena(st.st_mode, permisos);

        char usuario[32], grupo[32];
        obtener_propietario_y_grupo(st.st_uid, st.st_gid, usuario, sizeof(usuario), grupo, sizeof(grupo));

        /* Formatear fecha de modificacion */
        char fecha_str[20];
        struct tm *tm_info = localtime(&st.st_mtime);
        if (tm_info) {
            strftime(fecha_str, sizeof(fecha_str), "%Y-%m-%d %H:%M", tm_info);
        } else {
            snprintf(fecha_str, sizeof(fecha_str), "Desconocida");
        }

        /* Asignar color segun el tipo de archivo */
        const char *color = COLOR_RESET;
        if (S_ISDIR(st.st_mode)) {
            color = COLOR_AZUL;
        } else if (S_ISLNK(st.st_mode)) {
            color = COLOR_CYAN;
        } else if (st.st_mode & S_IXUSR) {
            color = COLOR_VERDE;
        }

        printf("%-11s %-12s %-12s %-10ld %-16s %s%s%s\n",
               permisos,
               usuario,
               grupo,
               (long)st.st_size,
               fecha_str,
               color, entry->d_name, COLOR_RESET);
    }

    printf("--------------------------------------------------------------------------------\n");
    closedir(dir);
    return 0;
}

int obtener_info_archivo(const char *ruta) {
    if (ruta == NULL || strlen(ruta) == 0) {
        fprintf(stderr, "Ruta no valida.\n");
        return -1;
    }

    struct stat st;
    if (lstat(ruta, &st) == -1) {
        perror("Error al consultar estado del archivo (lstat)");
        return -1;
    }

    char permisos[11];
    formato_permisos_cadena(st.st_mode, permisos);

    char usuario[32], grupo[32];
    obtener_propietario_y_grupo(st.st_uid, st.st_gid, usuario, sizeof(usuario), grupo, sizeof(grupo));

    char atime_str[32], mtime_str[32], ctime_str[32];
    strftime(atime_str, sizeof(atime_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_atime));
    strftime(mtime_str, sizeof(mtime_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    strftime(ctime_str, sizeof(ctime_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_ctime));

    printf("%s=== Informacion Detallada del Archivo ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Ruta            : %s\n", ruta);
    printf("  Numero de Inodo : %lu\n", (unsigned long)st.st_ino);
    printf("  Permisos        : %s (Octal: %04o)\n", permisos, st.st_mode & 07777);
    printf("  Propietario     : %s (UID: %u)\n", usuario, (unsigned int)st.st_uid);
    printf("  Grupo           : %s (GID: %u)\n", grupo, (unsigned int)st.st_gid);
    printf("  Tamano          : %ld bytes\n", (long)st.st_size);
    printf("  Enlaces duros   : %lu\n", (unsigned long)st.st_nlink);
    printf("  Ultimo Acceso   : %s\n", atime_str);
    printf("  Ultima Modif.   : %s\n", mtime_str);
    printf("  Cambio Estado   : %s\n", ctime_str);
    printf("==========================================\n");

    return 0;
}

int calcular_estadisticas_directorio(const char *ruta, EstadisticasDirectorio *stats) {
    if (ruta == NULL || stats == NULL) {
        return -1;
    }

    DIR *dir = opendir(ruta);
    if (!dir) {
        return -1;
    }

    struct dirent *entry;
    char path_completo[1024];

    while ((entry = readdir(dir)) != NULL) {
        /* Ignorar las rutas relativas "." y ".." para evitar recursion infinita */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path_completo, sizeof(path_completo), "%s/%s", ruta, entry->d_name);

        struct stat st;
        if (lstat(path_completo, &st) == -1) {
            continue;
        }

        stats->tamano_total_bytes += st.st_size;

        if (S_ISDIR(st.st_mode)) {
            stats->total_directorios++;
            /* Recorrido recursivo */
            calcular_estadisticas_directorio(path_completo, stats);
        } else if (S_ISREG(st.st_mode)) {
            stats->total_archivos++;
        } else if (S_ISLNK(st.st_mode)) {
            stats->total_enlaces++;
        } else {
            stats->total_otros++;
        }
    }

    closedir(dir);
    return 0;
}

void mostrar_estadisticas_directorio(const char *ruta) {
    const char *dir_base = (ruta != NULL && strlen(ruta) > 0) ? ruta : ".";

    EstadisticasDirectorio stats;
    memset(&stats, 0, sizeof(EstadisticasDirectorio));

    printf("%sAnalizando directorio recursivamente: %s ...%s\n", COLOR_AMARILLO, dir_base, COLOR_RESET);

    if (calcular_estadisticas_directorio(dir_base, &stats) != 0) {
        imprimir_mensaje("Error al calcular estadisticas del directorio.", COLOR_ROJO);
        return;
    }

    double tamano_mb = (double)stats.tamano_total_bytes / (1024.0 * 1024.0);

    printf("\n%s=== Estadisticas del Directorio ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Directorio Analizado : %s\n", dir_base);
    printf("  Archivos Regulares   : %zu\n", stats.total_archivos);
    printf("  Subdirectorios       : %zu\n", stats.total_directorios);
    printf("  Enlaces Simbolicos   : %zu\n", stats.total_enlaces);
    printf("  Otros Elementos      : %zu\n", stats.total_otros);
    printf("  Espacio Total        : %ld bytes (%.2f MB)\n", (long)stats.tamano_total_bytes, tamano_mb);
    printf("=========================================\n");
}

int cambiar_permisos_archivo(const char *ruta, mode_t modo_octal) {
    if (ruta == NULL) {
        return -1;
    }

    if (chmod(ruta, modo_octal) == -1) {
        perror("Error al cambiar permisos con chmod()");
        return -1;
    }

    printf("%sPermisos modificados exitosamente para '%s' -> (%04o)%s\n",
           COLOR_VERDE, ruta, modo_octal & 07777, COLOR_RESET);
    return 0;
}

int eliminar_elemento_recursivo(const char *ruta) {
    if (ruta == NULL || strlen(ruta) == 0) {
        return -1;
    }

    struct stat st;
    if (lstat(ruta, &st) == -1) {
        perror("Error al verificar la ruta a eliminar");
        return -1;
    }

    /* Si es un archivo o enlace simbolico, eliminar directamente con unlink */
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(ruta) == -1) {
            perror("Error al eliminar el archivo");
            return -1;
        }
        return 0;
    }

    /* Si es un directorio, vaciar contenido antes de borrar el directorio */
    DIR *dir = opendir(ruta);
    if (!dir) {
        perror("Error al abrir directorio para eliminado recursivo");
        return -1;
    }

    struct dirent *entry;
    char subruta[1024];
    int res = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(subruta, sizeof(subruta), "%s/%s", ruta, entry->d_name);
        res = eliminar_elemento_recursivo(subruta);
        if (res != 0) {
            break;
        }
    }

    closedir(dir);

    if (res == 0) {
        if (rmdir(ruta) == -1) {
            perror("Error al remover el directorio vacio");
            return -1;
        }
    }

    return res;
}