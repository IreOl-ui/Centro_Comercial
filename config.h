#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE "config.txt"

typedef struct {
    char admin_user[50];
    char admin_pass[50];
    char db_path[256];
    char log_path[256];
    int  max_intentos;
} Config;

int  config_cargar(Config* cfg, const char* filename);
// Lee config.txt y rellena la estructura Config
// Devuelve 1 si ok, 0 si error

void config_defaults(Config* cfg);
// Rellena Config con valores por defecto por si falta el fichero

#endif