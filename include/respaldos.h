#ifndef RESPALDOS_H
#define RESPALDOS_H

#include <sys/types.h>
#include <stddef.h>

/**
 * Estructura para registrar los resultados y contadores de una operacion de respaldo.
 */
typedef struct {
    size_t archivos_copiados;     /* Cantidad de archivos copiados o actualizados */
    size_t archivos_omitidos;     /* Cantidad de archivos sin cambios (omitidos) */
    size_t directorios_creados;   /* Cantidad de subdirectorios creados */
    off_t bytes_copiados;         /* Volumen total de datos transferidos en bytes */
    size_t errores;               /* Contador de fallos durante el proceso */
} EstadisticasRespaldo;

/**
 * Realiza un respaldo incremental recursivo desde un directorio origen hacia uno de destino.
 * @param origen Ruta del directorio origen.
 * @param destino Ruta del directorio donde se almacenara la copia de seguridad.
 * @param stats Puntero a la estructura donde se consolidaran las metricas.
 * @return 0 en exito total o parcial, -1 si los parametros son invalidos o falla el directorio raiz.
 */
int crear_respaldo_incremental(const char *origen, const char *destino, EstadisticasRespaldo *stats);

/**
 * Restaura el contenido de un respaldo copiando de forma completa sobre un directorio de destino.
 * @param origen_respaldo Ruta de la carpeta del respaldo a restaurar.
 * @param destino_restauracion Ruta del directorio donde se restauraran los archivos.
 * @param stats Puntero a la estructura de metricas del proceso.
 * @return 0 en exito, -1 en caso de fallo.
 */
int restaurar_respaldo(const char *origen_respaldo, const char *destino_restauracion, EstadisticasRespaldo *stats);

/**
 * Muestra en pantalla un resumen formateado de la operacion de respaldo/restauracion.
 * @param stats Puntero a la estructura con las metricas registradas.
 */
void mostrar_resumen_respaldo(const EstadisticasRespaldo *stats);

#endif /* RESPALDOS_H */