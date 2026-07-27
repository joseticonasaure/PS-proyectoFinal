#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

void limpiar_pantalla(void) {
    /* Secuencia ANSI: borrar pantalla completa y mover cursor a (1,1) */
    printf("\033[2J\033[H");
    fflush(stdout);
}

int obtener_ancho_terminal(void) {
#ifdef _WIN32
    return 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80; /* Valor por defecto ante fallos de consulta */
    }
    return (w.ws_col > 0) ? w.ws_col : 80;
#endif
}

void imprimir_encabezado(const char *titulo) {
    if (titulo == NULL) {
        return;
    }

    int ancho = obtener_ancho_terminal();
    if (ancho < 10) {
        ancho = 80;
    }

    printf("%s", COLOR_CYAN);
    for (int i = 0; i < ancho; i++) {
        putchar('=');
    }
    printf("\n");

    int len = (int)strlen(titulo);
    int espacios = (ancho - len) / 2;
    if (espacios < 0) {
        espacios = 0;
    }

    for (int i = 0; i < espacios; i++) {
        putchar(' ');
    }
    printf("%s%s%s\n", COLOR_NEGRITA, titulo, COLOR_CYAN);

    for (int i = 0; i < ancho; i++) {
        putchar('=');
    }
    printf("%s\n\n", COLOR_RESET);
    fflush(stdout);
}

void imprimir_mensaje(const char *mensaje, const char *color) {
    if (mensaje == NULL) {
        return;
    }
    const char *color_aplicar = (color != NULL) ? color : COLOR_RESET;
    printf("%s%s%s\n", color_aplicar, mensaje, COLOR_RESET);
    fflush(stdout);
}

void pausa_pantalla(void) {
    printf("\n%sPresione ENTER para continuar...%s", COLOR_AMARILLO, COLOR_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* Descartar caracteres remanentes en el buffer de entrada */
    }
}

char *trim_whitespace(char *str) {
    if (str == NULL) {
        return NULL;
    }

    /* Avanzar puntero ignorando espacios iniciales */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Retroceder desde el final eliminando espacios traseros */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

int leer_linea(char *buffer, size_t tamano) {
    if (buffer == NULL || tamano == 0) {
        return -1;
    }

    if (fgets(buffer, (int)tamano, stdin) == NULL) {
        buffer[0] = '\0';
        return -1;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        /* Si la linea sobrepasa el buffer, se purga el resto del stream stdin */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* Descartar */
        }
    }

    return 0;
}

void imprimir_barra_progreso(int actual, int total, int ancho_barra) {
    if (total <= 0 || ancho_barra <= 0) {
        return;
    }

    float porcentaje = (float)actual / (float)total;
    if (porcentaje > 1.0f) {
        porcentaje = 1.0f;
    }

    int llenado = (int)(porcentaje * ancho_barra);

    printf("\r[");
    for (int i = 0; i < ancho_barra; i++) {
        if (i < llenado) {
            printf("%s#%s", COLOR_VERDE, COLOR_RESET);
        } else {
            printf("-");
        }
    }
    printf("] %s%.1f%%%s", COLOR_AMARILLO, porcentaje * 100.0f, COLOR_RESET);
    fflush(stdout);

    if (actual >= total) {
        printf("\n");
    }
}