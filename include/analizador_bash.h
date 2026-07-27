#ifndef ANALIZADOR_BASH_H
#define ANALIZADOR_BASH_H

#include <stddef.h>

#define MAX_ADVERTENCIA_LEN 512

/**
 * Estructura para almacenar una advertencia o hallazgo de seguridad/sintaxis.
 */
typedef struct {
    int numero_linea;
    char mensaje[MAX_ADVERTENCIA_LEN];
    char nivel[16]; /* "INFO", "ALERTA", "PELIGRO" */
} AdvertenciaScript;

/**
 * Estructura que consolida el reporte completo del analisis de un script Bash.
 */
typedef struct {
    char ruta_archivo[256];
    int tiene_shebang;
    char shebang[128];
    size_t lineas_totales;
    size_t lineas_vacias;
    size_t lineas_comentarios;
    size_t variables_declaradas;
    size_t bucles_encontrados;
    size_t funciones_encontradas;
    
    AdvertenciaScript *advertencias;
    size_t num_advertencias;
    size_t cap_advertencias;
} ReporteAnalisisBash;

/**
 * Parsea un archivo de script Bash y genera las metricas y lista de advertencias.
 * @param ruta_script Ruta al archivo .sh a analizar.
 * @param reporte Puntero a la estructura donde se almacenaran los resultados.
 * @return 0 en exito, -1 en caso de error (ej. archivo inaccesible).
 */
int analizar_script_bash(const char *ruta_script, ReporteAnalisisBash *reporte);

/**
 * Imprime en la consola el reporte formateado con colores ANSI.
 * @param reporte Puntero al reporte a visualizar.
 */
void mostrar_reporte_bash(const ReporteAnalisisBash *reporte);

/**
 * Libera la memoria dinamica reservada para las advertencias del reporte.
 * @param reporte Puntero al reporte a liberar.
 */
void liberar_reporte_bash(ReporteAnalisisBash *reporte);

#endif /* ANALIZADOR_BASH_H */
