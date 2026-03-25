#ifndef MENU_H
#define MENU_H

#include "centro_comercial.h"

void menu_principal(CentroComercial* cc);
// Muestra el menú principal y gestiona la navegación


void menu_gestion_tiendas(CentroComercial* cc);
// Muestra el menú de gestión de tiendas


void menu_gestion_inventario(CentroComercial* cc);
// Muestra el menú de gestión de inventario de una tienda


void menu_gestion_cine(CentroComercial* cc);
// Muestra el menú de gestión de cine

#endif