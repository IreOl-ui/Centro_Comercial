#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "centro_comercial.h"

/*
 * persistencia.h  —  Capa de persistencia con SQLite3
 *
 * La interfaz pública es idéntica a la versión con fichero de texto,
 * por lo que main.c y menu.c NO necesitan ningún cambio.
 *
 * Base de datos SQLite generada (tablas):
 *
 *   tiendas   (id INTEGER PK, nombre TEXT)
 *   productos (id INTEGER PK, id_tienda INTEGER FK, nombre TEXT,
 *              precio REAL, stock INTEGER)
 *   peliculas (id INTEGER PK, titulo TEXT, sala INTEGER,
 *              horario TEXT, filas INTEGER, columnas INTEGER)
 *   reservas  (id_pelicula INTEGER FK, fila INTEGER, columna INTEGER,
 *              PRIMARY KEY (id_pelicula, fila, columna))
 */

int cargar_datos(CentroComercial* cc, const char* filename);
/* Abre (o crea) la base de datos SQLite en 'filename' y carga todos los
 * datos en la estructura en memoria.
 * Devuelve 1 si se cargaron datos, 0 si el archivo no existía o hubo error. */

int guardar_datos(const CentroComercial* cc, const char* filename);
/* Vuelca toda la estructura en memoria a la base de datos SQLite en 'filename'.
 * Sobreescribe los datos previos dentro de una única transacción.
 * Devuelve 1 si tuvo éxito, 0 si error. */

#endif /* PERSISTENCIA_H */