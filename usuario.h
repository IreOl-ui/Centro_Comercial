#ifndef USUARIO_H
#define USUARIO_H

#include "sqlite3.h"

typedef struct {
    int  id;
    char username[50];
    char password[50];
    char nombre[100];
} Usuario;

/* Crea la tabla usuarios si no existe */
int  usuario_crear_tabla(sqlite3* db);

/* Registra un nuevo usuario. Devuelve 1 OK, 0 si ya existe o error */
int  usuario_registrar(sqlite3* db, const char* username,
                       const char* password, const char* nombre);

/* Login: rellena 'out' si las credenciales son correctas.
 * Devuelve 1 si OK, 0 si no */
int  usuario_login(sqlite3* db, const char* username,
                   const char* password, Usuario* out);

/* Comprueba si ya existe un username. Devuelve 1 si existe */
int  usuario_existe(sqlite3* db, const char* username);

#endif