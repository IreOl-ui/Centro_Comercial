#ifndef LOG_H
#define LOG_H

void log_init(const char* filepath);
/* Abre (o crea) el fichero de log */

void log_escribir(const char* usuario, const char* accion);
/* Escribe una línea: [FECHA HORA] usuario: accion */

void log_cerrar(void);
/* Cierra el fichero de log */

#endif