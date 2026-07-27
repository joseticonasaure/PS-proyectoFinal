#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "utils.h"
#include "comandos.h"
#include "respaldos.h"
#include "analizador_bash.h"
#include "descargas.h"

/* Si tus fases previas definieron sistema.h / archivos.h / procesos.h, se incluyen aquí: */
#if __has_include("sistema.h")
#include "sistema.h"
#endif
#if __has_include("archivos.h")
#include "archivos.h"
#endif
#if __has_include("procesos.h")
#include "procesos.h"
#endif

/**
 * Auxiliar: Lee una línea de texto desde stdin de forma segura eliminando el salto de línea.
 */
static void leer_cadena(const char *prompt, char *buffer, size_t tamano) {
    printf("%s", prompt);
    if (fgets(buffer, (int)tamano, stdin) != NULL) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
    }
}

/**
 * Auxiliar: Lee un entero de forma segura desde la consola.
 */
static int leer_entero(const char *prompt) {
    char buffer[64];
    leer_cadena(prompt, buffer, sizeof(buffer));
    return atoi(buffer);
}

/* ============================================================================
 * MANIPULADORES DE SUBMENÚS Y MÓDULOS
 * ============================================================================ */

static void menu_ejecutar_comando(void) {
    limpiar_pantalla();
    imprimir_encabezado("EJECUCION DE COMANDOS EXTERNOS");

    char entrada[MAX_HISTORIAL_CMD];
    leer_cadena("Ingrese el comando a ejecutar (ej: 'ls -la' o 'date > salida.txt'): ", entrada, sizeof(entrada));

    if (strlen(entrada) == 0) {
        imprimir_mensaje("Comando vacio. Operacion cancelada.", COLOR_AMARILLO);
        pausa_pantalla();
        return;
    }

    /* Detección de redirecciones simples '>' o '>>' */
    char *redir_append = strstr(entrada, ">>");
    char *redir_trunc = strstr(entrada, ">");

    if (redir_append != NULL) {
        *redir_append = '\0';
        char *cmd = trim_whitespace(entrada);
        char *archivo = trim_whitespace(redir_append + 2);
        
        printf("%sEjecutando con redireccion (append >>)...%s\n", COLOR_CYAN, COLOR_RESET);
        ejecutar_comando_redireccionado(cmd, archivo, 1);
    } else if (redir_trunc != NULL) {
        *redir_trunc = '\0';
        char *cmd = trim_whitespace(entrada);
        char *archivo = trim_whitespace(redir_trunc + 1);

        printf("%sEjecutando con redireccion (sobrescribir >)...%s\n", COLOR_CYAN, COLOR_RESET);
        ejecutar_comando_redireccionado(cmd, archivo, 0);
    } else {
        printf("%sEjecutando comando...%s\n", COLOR_CYAN, COLOR_RESET);
        ejecutar_comando_externo(entrada);
    }

    pausa_pantalla();
}

static void menu_respaldos(void) {
    limpiar_pantalla();
    imprimir_encabezado("GESTION DE RESPALDOS INCREMENTALES");

    printf(" 1. Crear / Actualizar Respaldo Incremental\n");
    printf(" 2. Restaurar Respaldo\n");
    printf(" 0. Volver al menú principal\n");
    printf("--------------------------------------------\n");

    int opc = leer_entero("Seleccione una opción: ");
    if (opc == 0) return;

    char origen[256];
    char destino[256];
    EstadisticasRespaldo stats;

    if (opc == 1) {
        leer_cadena("Directorio Origen : ", origen, sizeof(origen));
        leer_cadena("Directorio Destino: ", destino, sizeof(destino));

        if (crear_respaldo_incremental(origen, destino, &stats) == 0) {
            mostrar_resumen_respaldo(&stats);
        }
    } else if (opc == 2) {
        leer_cadena("Directorio de Respaldo: ", origen, sizeof(origen));
        leer_cadena("Directorio de Destino : ", destino, sizeof(destino));

        if (restaurar_respaldo(origen, destino, &stats) == 0) {
            mostrar_resumen_respaldo(&stats);
        }
    } else {
        imprimir_mensaje("Opcion no valida.", COLOR_ROJO);
    }

    pausa_pantalla();
}

static void menu_analizador_bash(void) {
    limpiar_pantalla();
    imprimir_encabezado("ANALIZADOR ESTATICO DE SCRIPTS BASH");

    char ruta[256];
    leer_cadena("Ingrese la ruta del script Shell (.sh): ", ruta, sizeof(ruta));

    if (strlen(ruta) == 0) {
        imprimir_mensaje("Ruta vacia. Operacion cancelada.", COLOR_AMARILLO);
        pausa_pantalla();
        return;
    }

    ReporteAnalisisBash reporte;
    if (analizar_script_bash(ruta, &reporte) == 0) {
        mostrar_reporte_bash(&reporte);
        liberar_reporte_bash(&reporte);
    } else {
        imprimir_mensaje("No se pudo abrir ni analizar el archivo especificado.", COLOR_ROJO);
    }

    pausa_pantalla();
}

static void menu_descargas(void) {
    int opc = -1;
    while (opc != 0) {
        limpiar_pantalla();
        imprimir_encabezado("COLA MULTIHILO DE DESCARGAS (PTHREADS)");

        printf(" 1. Agregar Nueva Tarea de Descarga\n");
        printf(" 2. Monitorear Estado de la Cola en Vivo\n");
        printf(" 0. Volver al Menu Principal\n");
        printf("--------------------------------------------\n");

        opc = leer_entero("Seleccione una opcion: ");

        if (opc == 1) {
            char url[MAX_URL_LEN];
            char dest[MAX_PATH_LEN];
            leer_cadena("URL u Origen del Recurso: ", url, sizeof(url));
            leer_cadena("Ruta Local de Destino   : ", dest, sizeof(dest));
            int tam_mb = leer_entero("Tamano simulacion (MB)  : ");

            if (tam_mb <= 0) tam_mb = 2;
            size_t bytes = (size_t)tam_mb * 1024 * 1024;

            int id = agregar_tarea_descarga(url, dest, bytes);
            if (id > 0) {
                printf("%sTarea agregada exitosamente con ID [%d].%s\n", COLOR_VERDE, id, COLOR_RESET);
            }
            pausa_pantalla();
        } else if (opc == 2) {
            int segundos = leer_entero("Segundos de monitoreo continuo (ej: 5): ");
            if (segundos <= 0) segundos = 5;

            for (int i = 0; i < segundos * 2; i++) {
                limpiar_pantalla();
                imprimir_encabezado("MONITOREO EN VIVO - DESCARGAS PTHREADS");
                mostrar_estado_cola();
                printf("\nPresione Ctrl+C si desea interrumpir antes...\n");
                usleep(500000); /* Refresco cada 500 ms */
            }
            pausa_pantalla();
        }
    }
}

static void menu_historial(void) {
    limpiar_pantalla();
    imprimir_encabezado("HISTORIAL DE COMANDOS REGISTRADOS");

    mostrar_historial();

    printf("\n 1. Guardar Historial en Disco\n");
    printf(" 2. Vaciar Historial de Memoria\n");
    printf(" 0. Volver\n");
    printf("--------------------------------------------\n");

    int opc = leer_entero("Opcion: ");
    if (opc == 1) {
        guardar_historial_archivo(NULL);
        pausa_pantalla();
    } else if (opc == 2) {
        vaciar_historial();
        pausa_pantalla();
    }
}

/* ============================================================================
 * FUNCIÓN PRINCIPAL (MAIN)
 * ============================================================================ */

int main(void) {
    /* 1. Inicialización de componentes */
    cargar_historial_archivo(NULL);      /* Cargar historial previo si existe */
    inicializar_cola_descargas(2);       /* Inicializar pool con 2 hilos */

    int opcion = -1;

    while (opcion != 0) {
        limpiar_pantalla();
        imprimir_encabezado("SISTEMA INTEGRADO DE ADMINISTRACION LINUX (ADMIN_SYS)");

        printf(" %s1.%s Ejecutar Comando Externo / Redireccion\n", COLOR_CYAN, COLOR_RESET);
        printf(" %s2.%s Respaldos Incrementales y Restauracion\n", COLOR_CYAN, COLOR_RESET);
        printf(" %s3.%s Analisis Estatico de Scripts Bash (.sh)\n", COLOR_CYAN, COLOR_RESET);
        printf(" %s4.%s Gestor Multihilo de Descargas (pthreads)\n", COLOR_CYAN, COLOR_RESET);
        printf(" %s5.%s Ver / Gestionar Historial de Comandos\n", COLOR_CYAN, COLOR_RESET);
        printf(" %s0.%s Salir del Sistema\n", COLOR_ROJO, COLOR_RESET);
        printf("==================================================================\n");

        opcion = leer_entero(" Seleccione una opcion del menu: ");

        switch (opcion) {
            case 1:
                menu_ejecutar_comando();
                break;
            case 2:
                menu_respaldos();
                break;
            case 3:
                menu_analizador_bash();
                break;
            case 4:
                menu_descargas();
                break;
            case 5:
                menu_historial();
                break;
            case 0:
                limpiar_pantalla();
                imprimir_mensaje("Guardando datos y cerrando servicios...", COLOR_AMARILLO);
                guardar_historial_archivo(NULL);
                destruir_cola_descargas();
                vaciar_historial();
                imprimir_mensaje("Gracias por utilizar Admin_Sys! Hasta luego.", COLOR_VERDE);
                break;
            default:
                imprimir_mensaje("Opcion no valida. Intente nuevamente.", COLOR_ROJO);
                pausa_pantalla();
                break;
        }
    }

    return 0;
}