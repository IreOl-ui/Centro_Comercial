#include "centro_comercial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CentroComercial* cc_crear(void) {
    CentroComercial* cc = (CentroComercial*)malloc(sizeof(CentroComercial));
    if (cc == NULL) {
        return NULL;
    }
    
    cc->listaTiendas = NULL;
    cc->numTiendas = 0;
    cc->cartelera = NULL;
    cc->numPeliculas = 0;
    
    return cc;
}

void cc_liberar(CentroComercial* cc) {
    if (cc != NULL) {
        // Liberar todas las tiendas
        if (cc->listaTiendas != NULL) {
            for (int i = 0; i < cc->numTiendas; i++) {
                tienda_liberar(cc->listaTiendas[i]);
            }
            free(cc->listaTiendas);
        }
        
        // Liberar todas las películas
        if (cc->cartelera != NULL) {
            for (int i = 0; i < cc->numPeliculas; i++) {
                pelicula_liberar(cc->cartelera[i]);
            }
            free(cc->cartelera);
        }
        
        free(cc);
    }
}

int cc_agregarTienda(CentroComercial* cc, Tienda* tienda) {
    if (cc == NULL || tienda == NULL) {
        return 0;
    }
    
    // Verificar que no existe ya una tienda con el mismo ID
    if (cc_buscarTiendaPorId(cc, tienda->id) != NULL) {
        return 0;
    }
    
    Tienda** nuevoArray = (Tienda**)realloc(cc->listaTiendas, 
                                (cc->numTiendas + 1) * sizeof(Tienda*));
    if (nuevoArray == NULL) {
        return 0;
    }
    
    cc->listaTiendas = nuevoArray;
    cc->listaTiendas[cc->numTiendas] = tienda;
    cc->numTiendas++;
    
    return 1;
}

int cc_eliminarTienda(CentroComercial* cc, int idTienda) {
    if (cc == NULL || cc->listaTiendas == NULL) {
        return 0;
    }
    
    for (int i = 0; i < cc->numTiendas; i++) {
        if (cc->listaTiendas[i]->id == idTienda) {
            tienda_liberar(cc->listaTiendas[i]);
            
            for (int j = i; j < cc->numTiendas - 1; j++) {
                cc->listaTiendas[j] = cc->listaTiendas[j + 1];
            }
            
            cc->numTiendas--;
            
            if (cc->numTiendas == 0) {
                free(cc->listaTiendas);
                cc->listaTiendas = NULL;
            } else {
                Tienda** nuevoArray = (Tienda**)realloc(cc->listaTiendas, 
                                            cc->numTiendas * sizeof(Tienda*));
                if (nuevoArray != NULL) {
                    cc->listaTiendas = nuevoArray;
                }
            }
            return 1;
        }
    }
    return 0;
}

Tienda* cc_buscarTiendaPorId(const CentroComercial* cc, int idTienda) {
    if (cc == NULL || cc->listaTiendas == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < cc->numTiendas; i++) {
        if (cc->listaTiendas[i]->id == idTienda) {
            return cc->listaTiendas[i];
        }
    }
    return NULL;
}

int cc_agregarPelicula(CentroComercial* cc, Pelicula* pelicula) {
    if (cc == NULL || pelicula == NULL) {
        return 0;
    }
    
    // Verificar que no existe otra película en la misma sala y horario
    for (int i = 0; i < cc->numPeliculas; i++) {
        if (cc->cartelera[i]->sala == pelicula->sala && 
            strcmp(cc->cartelera[i]->horario, pelicula->horario) == 0) {
            return 0;
        }
    }
    
    Pelicula** nuevoArray = (Pelicula**)realloc(cc->cartelera, 
                                 (cc->numPeliculas + 1) * sizeof(Pelicula*));
    if (nuevoArray == NULL) {
        return 0;
    }
    
    cc->cartelera = nuevoArray;
    cc->cartelera[cc->numPeliculas] = pelicula;
    cc->numPeliculas++;
    
    return 1;
}

int cc_eliminarPelicula(CentroComercial* cc, int idPelicula) {
    if (cc == NULL || cc->cartelera == NULL) {
        return 0;
    }
    
    for (int i = 0; i < cc->numPeliculas; i++) {
        if (cc->cartelera[i]->id == idPelicula) {
            pelicula_liberar(cc->cartelera[i]);
            
            for (int j = i; j < cc->numPeliculas - 1; j++) {
                cc->cartelera[j] = cc->cartelera[j + 1];
            }
            
            cc->numPeliculas--;
            
            if (cc->numPeliculas == 0) {
                free(cc->cartelera);
                cc->cartelera = NULL;
            } else {
                Pelicula** nuevoArray = (Pelicula**)realloc(cc->cartelera, 
                                            cc->numPeliculas * sizeof(Pelicula*));
                if (nuevoArray != NULL) {
                    cc->cartelera = nuevoArray;
                }
            }
            return 1;
        }
    }
    return 0;
}

Pelicula* cc_buscarPeliculaPorId(const CentroComercial* cc, int idPelicula) {
    if (cc == NULL || cc->cartelera == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < cc->numPeliculas; i++) {
        if (cc->cartelera[i]->id == idPelicula) {
            return cc->cartelera[i];
        }
    }
    return NULL;
}

int cc_getNumTiendas(const CentroComercial* cc) {
    return cc->numTiendas;
}

int cc_getNumPeliculas(const CentroComercial* cc) {
    return cc->numPeliculas;
}