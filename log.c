#include "log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static FILE* g_log_file = NULL;

void log_init(const char* filepath) {
    if (filepath == NULL || strlen(filepath) == 0) {
        g_log_file = fopen("deusto_centro.log", "a");
    } else {
        g_log_file = fopen(filepath, "a");
    }
}

void log_escribir(const char* usuario, const char* accion) {
    if (g_log_file == NULL) return;

    time_t ahora = time(NULL);
    struct tm* t  = localtime(&ahora);
    char fecha[32];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t);

    fprintf(g_log_file, "[%s] %-15s %s\n", fecha, usuario, accion);
    fflush(g_log_file);
}

void log_cerrar(void) {
    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}