#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "centro_comercial.h"

int cargar_datos(CentroComercial* cc, const char* filename);

int guardar_datos(const CentroComercial* cc, const char* filename);

#endif 