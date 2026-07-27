#include "analizador_bash.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/**
 * Auxiliar: Agrega una advertencia o hallazgo al arreglo dinamico del reporte.
 */
static void agregar_advertencia(ReporteAnalisisBash *reporte, int linea, const char *nivel, const char *mensaje) {
    if (!reporte || !mensaje || !nivel) {
        return;
    }

    if (reporte->num_advertencias >= reporte->cap_advertencias) {
        size_t nueva_cap = (reporte->cap_advertencias == 0) ? 8 : reporte->cap_advertencias * 2;
        AdvertenciaScript *temp = realloc(reporte->advertencias, nueva_cap * sizeof(AdvertenciaScript));
        if (!temp) {
            perror("Error reasignando memoria para advertencias de script");
            return;
        }
        reporte->advertencias = temp;
        reporte->cap_advertencias = nueva_cap;
    }

    AdvertenciaScript *adv = &reporte->advertencias[reporte->num_advertencias];
    adv->numero_linea = linea;
    strncpy(adv->nivel, nivel, sizeof(adv->nivel) - 1);
    adv->nivel[sizeof(adv->nivel) - 1] = '\0';

    strncpy(adv->mensaje, mensaje, sizeof(adv->mensaje) - 1);
    adv->mensaje[sizeof(adv->mensaje) - 1] = '\0';

    reporte->num_advertencias++;
}

/**
 * Auxiliar: Comprueba si un identificador de variable es valido en Bash.
 */
static int es_inicio_variable(const char *str) {
    if (!str || !isalpha((unsigned char)*str) && *str != '_') {
        return 0;
    }

    const char *p = str;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
        p++;
    }

    return (*p == '=');
}

int analizar_script_bash(const char *ruta_script, ReporteAnalisisBash *reporte) {
    if (!ruta_script || !reporte) {
        return -1;
    }

    FILE *f = fopen(ruta_script, "r");
    if (!f) {
        perror("Error al abrir script Bash para analisis");
        return -1;
    }

    memset(reporte, 0, sizeof(ReporteAnalisisBash));
    strncpy(reporte->ruta_archivo, ruta_script, sizeof(reporte->ruta_archivo) - 1);

    char linea[1024];
    int num_linea = 0;

    while (fgets(linea, sizeof(linea), f)) {
        num_linea++;
        reporte->lineas_totales++;

        /* 1. Analisis de Shebang en la primera linea */
        if (num_linea == 1) {
            if (strncmp(linea, "#!", 2) == 0) {
                reporte->tiene_shebang = 1;
                char *trim_linea = trim_whitespace(linea);
                strncpy(reporte->shebang, trim_linea, sizeof(reporte->shebang) - 1);
            } else {
                agregar_advertencia(reporte, 1, "ALERTA", "Falta el encabezado Shebang (ej: #!/bin/bash).");
            }
        }

        char *linea_limpia = trim_whitespace(linea);

        /* 2. Lineas vacias */
        if (strlen(linea_limpia) == 0) {
            reporte->lineas_vacias++;
            continue;
        }

        /* 3. Comentarios */
        if (linea_limpia[0] == '#') {
            if (num_linea != 1 || !reporte->tiene_shebang) {
                reporte->lineas_comentarios++;
            }
            continue;
        }

        /* 4. Deteccion de Declaracion de Variables (ej: MI_VAR=valor) */
        if (es_inicio_variable(linea_limpia)) {
            reporte->variables_declaradas++;
        }

        /* 5. Deteccion de Estructuras de Control / Bucles */
        if (strstr(linea_limpia, "while ") || strstr(linea_limpia, "for ") || strstr(linea_limpia, "until ")) {
            reporte->bucles_encontrados++;
        }

        /* 6. Deteccion de Funciones */
        if (strstr(linea_limpia, "function ") || (strstr(linea_limpia, "()") && strstr(linea_limpia, "{"))) {
            reporte->funciones_encontradas++;
        }

        /* 7. Analisis de Seguridad y Riesgos Potenciales */
        if (strstr(linea_limpia, "rm -rf /") || strstr(linea_limpia, "rm -rf *")) {
            agregar_advertencia(reporte, num_linea, "PELIGRO", "Comando de borrado masivo potencialmente destructivo detected (rm -rf).");
        }

        if (strstr(linea_limpia, "chmod 777")) {
            agregar_advertencia(reporte, num_linea, "ALERTA", "Asignacion de permisos inseguros 'chmod 777'.");
        }

        if (strstr(linea_limpia, "eval ")) {
            agregar_advertencia(reporte, num_linea, "ALERTA", "Uso de 'eval' detectado. Riesgo potencial de inyeccion de comandos.");
        }

        if ((strstr(linea_limpia, "curl") || strstr(linea_limpia, "wget")) &&
            (strstr(linea_limpia, "| bash") || strstr(linea_limpia, "| sh"))) {
            agregar_advertencia(reporte, num_linea, "PELIGRO", "Ejecucion remota directa mediante tuberia (curl/wget | bash).");
        }

        if (strstr(linea_limpia, "sudo ")) {
            agregar_advertencia(reporte, num_linea, "INFO", "Uso de elevacion de privilegios 'sudo'.");
        }
    }

    fclose(f);
    return 0;
}

void mostrar_reporte_bash(const ReporteAnalisisBash *reporte) {
    if (!reporte) {
        return;
    }

    printf("%s=== Reporte del Analisis del Script Bash ===%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Archivo Analizado    : %s\n", reporte->ruta_archivo);
    printf("  Shebang Detectado    : %s\n", reporte->tiene_shebang ? reporte->shebang : "NO DETECTADO");
    printf("  Lineas Totales       : %zu\n", reporte->lineas_totales);
    printf("  Lineas Vacias        : %zu\n", reporte->lineas_vacias);
    printf("  Lineas de Comentarios: %zu\n", reporte->lineas_comentarios);
    printf("  Variables Declaradas : %zu\n", reporte->variables_declaradas);
    printf("  Bucles Detectados    : %zu\n", reporte->bucles_encontrados);
    printf("  Funciones Definidas  : %zu\n", reporte->funciones_encontradas);
    printf("--------------------------------------------\n");

    if (reporte->num_advertencias == 0) {
        printf("%sNo se encontraron problemas ni riesgos de seguridad.%s\n", COLOR_VERDE, COLOR_RESET);
    } else {
        printf("%sHallazgos y Advertencias (%zu):%s\n", COLOR_AMARILLO, reporte->num_advertencias, COLOR_RESET);
        for (size_t i = 0; i < reporte->num_advertencias; i++) {
            const AdvertenciaScript *adv = &reporte->advertencias[i];
            const char *color_nivel = COLOR_RESET;

            if (strcmp(adv->nivel, "PELIGRO") == 0) {
                color_nivel = COLOR_ROJO;
            } else if (strcmp(adv->nivel, "ALERTA") == 0) {
                color_nivel = COLOR_AMARILLO;
            } else {
                color_nivel = COLOR_AZUL;
            }

            printf("  Linea %-4d [%s%s%s]: %s\n",
                   adv->numero_linea,
                   color_nivel, adv->nivel, COLOR_RESET,
                   adv->mensaje);
        }
    }
    printf("============================================\n");
}

void liberar_reporte_bash(ReporteAnalisisBash *reporte) {
    if (reporte && reporte->advertencias) {
        free(reporte->advertencias);
        reporte->advertencias = NULL;
        reporte->num_advertencias = 0;
        reporte->cap_advertencias = 0;
    }
}