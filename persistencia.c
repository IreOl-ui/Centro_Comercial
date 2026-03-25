#include "persistencia.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINEA 512

int cargar_datos(CentroComercial* cc, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }
    
    char linea[MAX_LINEA];
    char seccion[20] = "";
    Tienda* tiendaActual = NULL;
    Pelicula* peliculaActual = NULL;
    
    while (fgets(linea, sizeof(linea), file)) {
        // Eliminar salto de línea
        linea[strcspn(linea, "\n")] = '\0';
        
        if (linea[0] == '#') {
            strcpy(seccion, linea);
            continue;
        }
        
        if (strcmp(seccion, "#TIENDAS") == 0) {
            if (strchr(linea, ',') != NULL && linea[0] != 'P') {
                // Es una tienda
                int id;
                char nombre[100];
                sscanf(linea, "%d,%99[^,]", &id, nombre);
                Tienda* tienda = tienda_crear(id, nombre);
                if (tienda != NULL) {
                    cc_agregarTienda(cc, tienda);
                    tiendaActual = tienda;
                }
            } else if (linea[0] == 'P' && tiendaActual != NULL) {
                // Es un producto
                int idProducto, stock;
                char nombre[100];
                float precio;
                sscanf(linea, "P,%d,%99[^,],%f,%d", &idProducto, nombre, &precio, &stock);
                Producto* producto = producto_crear(idProducto, nombre, precio, stock);
                if (producto != NULL) {
                    tienda_aniadirProducto(tiendaActual, producto);
                }
            }
        } else if (strcmp(seccion, "#CINE") == 0) {
            if (linea[0] != 'R' && linea[0] != '\0') {
                // Es una película
                int id, sala, filas, columnas;
                char titulo[200], horario[20];
                sscanf(linea, "%d,%199[^,],%d,%19[^,],%d,%d", 
                       &id, titulo, &sala, horario, &filas, &columnas);
                Pelicula* pelicula = pelicula_crear(id, titulo, sala, horario, filas, columnas);
                if (pelicula != NULL) {
                    cc_agregarPelicula(cc, pelicula);
                    peliculaActual = pelicula;
                }
            } else if (linea[0] == 'R' && peliculaActual != NULL) {
                // Es una reserva
                int fila, columna;
                sscanf(linea, "R,%d,%d", &fila, &columna);
                // Convertir de 1-indexado a 0-indexado
                pelicula_reservarAsiento(peliculaActual, fila - 1, columna - 1);
            }
        }
    }
    
    fclose(file);
    return 1;
}

int guardar_datos(const CentroComercial* cc, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        return 0;
    }
    
    // Sección Tiendas
    fprintf(file, "#TIENDAS\n");
    for (int i = 0; i < cc->numTiendas; i++) {
        Tienda* tienda = cc->listaTiendas[i];
        fprintf(file, "%d,%s\n", tienda->id, tienda->nombre);
        
        for (int j = 0; j < tienda->numProductos; j++) {
            Producto* producto = tienda->inventario[j];
            fprintf(file, "P,%d,%s,%.2f,%d\n", 
                    producto->id, producto->nombre, producto->precio, producto->stock);
        }
    }
    
    // Sección Cine
    fprintf(file, "#CINE\n");
    for (int i = 0; i < cc->numPeliculas; i++) {
        Pelicula* pelicula = cc->cartelera[i];
        fprintf(file, "%d,%s,%d,%s,%d,%d\n", 
                pelicula->id, pelicula->titulo, pelicula->sala, 
                pelicula->horario, pelicula->filas, pelicula->columnas);
        
        // Guardar reservas (asientos ocupados)
        for (int f = 0; f < pelicula->filas; f++) {
            for (int c = 0; c < pelicula->columnas; c++) {
                if (pelicula->asientos[f][c] == 1) {
                    // Guardar en 1-indexado para el archivo
                    fprintf(file, "R,%d,%d\n", f + 1, c + 1);
                }
            }
        }
    }
    
    fclose(file);
    return 1;
}