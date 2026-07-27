#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Codigos de colores ANSI para la interfaz en terminal */
#define COLOR_RESET    "\x1b[0m"
#define COLOR_ROJO     "\x1b[31m"
#define COLOR_VERDE    "\x1b[32m"
#define COLOR_AMARILLO "\x1b[33m"
#define COLOR_AZUL     "\x1b[34m"
#define COLOR_MAGENTA  "\x1b[35m"
#define COLOR_CYAN     "\x1b[36m"
#define COLOR_BLANCO   "\x1b[37m"
#define COLOR_NEGRITA  "\x1b[1m"

/* Prototipos de funciones auxiliares y de interfaz */

/**
 * Limpia la pantalla de la terminal posicionando el cursor al inicio.
 */
void limpiar_pantalla(void);

/**
 * Imprime un encabezado centrado segun el ancho dinamico de la terminal.
 * @param titulo Texto del encabezado a mostrar.
 */
void imprimir_encabezado(const char *titulo);

/**
 * Imprime un mensaje con formato de color ANSI especifico.
 * @param mensaje Texto a imprimir.
 * @param color Cadena ANSI de color definida en utils.h.
 */
void imprimir_mensaje(const char *mensaje, const char *color);

/**
 * Pausa la ejecucion del programa hasta que el usuario presione ENTER.
 */
void pausa_pantalla(void);

/**
 * Elimina espacios en blanco al inicio y al final de una cadena de texto.
 * Modifica la cadena original y retorna el puntero ajustado.
 * @param str Cadena de texto a procesar.
 * @return Puntero al primer caracter no blanco de la cadena.
 */
char *trim_whitespace(char *str);

/**
 * Lee una linea completa desde stdin de forma segura evitando desbordamiento de buffer.
 * @param buffer Arreglo donde se almacenara la entrada.
 * @param tamano Capacidad maxima del buffer.
 * @return 0 en exito, -1 en caso de error o fin de archivo (EOF).
 */
int leer_linea(char *buffer, size_t tamano);

/**
 * Consulta la API POSIX de la terminal para obtener su ancho en columnas.
 * @return Numero de columnas de la terminal, o 80 por defecto si falla la llamada ioctl.
 */
int obtener_ancho_terminal(void);

/**
 * Dibuja una barra de progreso ANSI interactiva en la consola.
 * @param actual Valor actual completado.
 * @param total Valor objetivo final.
 * @param ancho_barra Ancho total en caracteres para renderizar la barra.
 */
void imprimir_barra_progreso(int actual, int total, int ancho_barra);

#endif /* UTILS_H */