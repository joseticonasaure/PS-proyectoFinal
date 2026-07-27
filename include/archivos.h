#ifndef ARCHIVOS_H
#define ARCHIVOS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

/**
 * Estructura para almacenar las estadisticas acumuladas de un directorio.
 */
typedef struct {
    off_t tamano_total_bytes;    /* Tamano acumulado en bytes */
    size_t total_archivos;        /* Cantidad de archivos regulares */
    size_t total_directorios;     /* Cantidad de subdirectorios */
    size_t total_enlaces;         /* Cantidad de enlaces simbolicos */
    size_t total_otros;           /* Dispositivos, sockets, FIFOs, etc. */
} EstadisticasDirectorio;

/**
 * Lista el contenido de un directorio mostrando permisos, propietario, tamano y tipo.
 * @param ruta Ruta del directorio a listar.
 * @return 0 en exito, -1 en caso de error.
 */
int listar_directorio(const char *ruta);

/**
 * Muestra los metadatos detallados de un archivo o directorio (inodo, permisos octales, fechas, etc.).
 * @param ruta Ruta del archivo o directorio.
 * @return 0 en exito, -1 en caso de error.
 */
int obtener_info_archivo(const char *ruta);

/**
 * Recorre recursivamente un directorio para calcular su tamano total y contadores de elementos.
 * @param ruta Ruta del directorio base.
 * @param stats Puntero a la estructura donde se guardaran las metricas.
 * @return 0 en exito, -1 en caso de error.
 */
int calcular_estadisticas_directorio(const char *ruta, EstadisticasDirectorio *stats);

/**
 * Cambia los permisos de un archivo o directorio usando chmod en notacion octal.
 * @param ruta Ruta del elemento a modificar.
 * @param modo_octal Modo de permisos en Notacion Octal (ej: 0755, 0644).
 * @return 0 en exito, -1 en caso de error.
 */
int cambiar_permisos_archivo(const char *ruta, mode_t modo_octal);

/**
 * Elimina un archivo regular o un directorio de forma recursiva.
 * @param ruta Ruta del elemento a eliminar.
 * @return 0 en exito, -1 en caso de error.
 */
int eliminar_elemento_recursivo(const char *ruta);

/**
 * Muestra en consola las estadisticas calculadas de un directorio de forma formateada.
 * @param ruta Ruta del directorio a analizar.
 */
void mostrar_estadisticas_directorio(const char *ruta);

#endif /* ARCHIVOS_H */