#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void config_defaults(Config* cfg) {
    strcpy(cfg->admin_user,  "admin");
    strcpy(cfg->admin_pass,  "1234");
    strcpy(cfg->db_path,     "deusto_centro_data.db");
    cfg->max_intentos = 3;
}

int config_cargar(Config* cfg, const char* filename) {
    config_defaults(cfg);   /* Partir siempre de valores seguros */

    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("[config] '%s' no encontrado. Usando valores por defecto.\n", filename);
        return 0;
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        /* Ignorar comentarios y líneas vacías */
        if (linea[0] == '#' || linea[0] == '\n' || linea[0] == '\r')
            continue;

        /* Eliminar salto de línea */
        linea[strcspn(linea, "\r\n")] = '\0';

        char clave[64], valor[256];
        if (sscanf(linea, "%63[^=]=%255s", clave, valor) != 2)
            continue;

        if (strcmp(clave, "ADMIN_USER")   == 0) strncpy(cfg->admin_user,  valor, 49);
        else if (strcmp(clave, "ADMIN_PASS")   == 0) strncpy(cfg->admin_pass,  valor, 49);
        else if (strcmp(clave, "DB_PATH")      == 0) strncpy(cfg->db_path,     valor, 255);
        else if (strcmp(clave, "MAX_INTENTOS") == 0) cfg->max_intentos = atoi(valor);
    }

    fclose(f);
    return 1;
}