#ifndef COMANDOS_H
#define COMANDOS_H

#include <sys/types.h>
#include <time.h>
#include <stddef.h>

#define MAX_HISTORIAL_CMD 512
#define ARCHIVO_HISTORIAL_DEF ".admin_historial.txt"

/**
 * Estructura para registrar una entrada en el historial de comandos ejecutados.
 */
typedef struct {
    char comando[MAX_HISTORIAL_CMD];  /* Cadena del comando ejecutado */
    time_t fecha_hora;               /* Marca de tiempo Unix */
    int codigo_salida;               /* Codigo de retorno del proceso hijo (exit status) */
    double tiempo_ejecucion_seg;     /* Duracion de la ejecucion en segundos */
} EntradaHistorial;

/**
 * Ejecuta un comando externo mediante fork() y execvp(), midiendo su tiempo y registrandolo en historial.
 * @param linea_comando Cadena con el comando y sus argumentos.
 * @return Codigo de salida del comando (0 en exito, !=0 en error).
 */
int ejecutar_comando_externo(const char *linea_comando);

/**
 * Ejecuta un comando redirigiendo stdout hacia un archivo (> o >>).
 * @param linea_comando Comando a ejecutar.
 * @param archivo_salida Ruta del archivo destino de la redireccion.
 * @param es_append 0 para sobrescribir (>), 1 para agregar al final (>>).
 * @return Codigo de salida del comando.
 */
int ejecutar_comando_redireccionado(const char *linea_comando, const char *archivo_salida, int es_append);

/**
 * Agrega un registro manualmente a la estructura en memoria del historial.
 * @param comando Texto del comando.
 * @param codigo_salida Estado de finalizacion.
 * @param tiempo_ejecucion Tiempo transcurrido en segundos.
 * @return 0 en exito, -1 en caso de fallo.
 */
int agregar_a_historial(const char *comando, int codigo_salida, double tiempo_ejecucion);

/**
 * Imprime en la consola la lista formateada del historial de comandos.
 */
void mostrar_historial(void);

/**
 * Guarda el historial cargado en memoria hacia un archivo de texto en disco.
 * @param ruta_archivo Ruta del archivo de destino (NULL para usar valor por defecto).
 * @return 0 en exito, -1 en caso de error.
 */
int guardar_historial_archivo(const char *ruta_archivo);

/**
 * Carga el historial previo guardado en disco hacia la memoria principal.
 * @param ruta_archivo Ruta del archivo a leer (NULL para usar valor por defecto).
 * @return 0 en exito, -1 en caso de error o si el archivo no existe.
 */
int cargar_historial_archivo(const char *ruta_archivo);

/**
 * Libera la memoria dinamica ocupada por el historial.
 */
void vaciar_historial(void);

#endif /* COMANDOS_H */