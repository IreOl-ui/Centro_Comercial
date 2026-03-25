#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "centro_comercial.h"

int cargar_datos(CentroComercial* cc, const char* filename);
// Carga los datos del centro comercial desde el archivo de texto
// Devuelve 1, y 0 si error

int guardar_datos(const CentroComercial* cc, const char* filename);
// Guarda los datos del centro comercial en el archivo de texto
// Devuelve 1, y 0 si error
#endif