#ifndef CENTRO_COMERCIAL_H
#define CENTRO_COMERCIAL_H

#include "tienda.h"
#include "pelicula.h"

typedef struct CentroComercial {
    Tienda** listaTiendas;
    int numTiendas;
    Pelicula** cartelera;
    int numPeliculas;
} CentroComercial;


CentroComercial* cc_crear(void);
// Crea un nuevo centro comercial vacío
// Devuelve un puntero al centro comercial, NULL si falla


void cc_liberar(CentroComercial* cc);
// Libera toda la memoria asociada al centro comercial 


int cc_agregarTienda(CentroComercial* cc, Tienda* tienda);
// Añade una tienda al centro comercial
// Devuelve 1, y 0 si falla


int cc_eliminarTienda(CentroComercial* cc, int idTienda);
// Elimina una tienda del centro comercial por su ID
// Devuelve 1, y 0 si no se encontró


Tienda* cc_buscarTiendaPorId(const CentroComercial* cc, int idTienda);
// Busca una tienda por su ID
// Devuelve un puntero a la tienda, NULL si no existe


int cc_agregarPelicula(CentroComercial* cc, Pelicula* pelicula);
// Añade una película a la cartelera
// Devuelve 1, y 0 si falla


int cc_eliminarPelicula(CentroComercial* cc, int idPelicula);
// Elimina una película de la cartelera por su ID
// Devuleve 1, y 0 si falla


Pelicula* cc_buscarPeliculaPorId(const CentroComercial* cc, int idPelicula);
// Busca una película por su ID
// Devuelve un puntero a la película, NULL si no existe


int cc_getNumTiendas(const CentroComercial* cc);
// Obtiene el número de tiendas


int cc_getNumPeliculas(const CentroComercial* cc);
// Obtiene el número de películas

#endif