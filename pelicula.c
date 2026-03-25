#include "pelicula.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Pelicula* pelicula_crear(int id, const char* titulo, int sala, const char* horario,
                         int filas, int columnas) {
    Pelicula* pelicula = (Pelicula*)malloc(sizeof(Pelicula));
    if (pelicula == NULL) {
        return NULL;
    }
    
    pelicula->id = id;
    pelicula->titulo = (char*)malloc(strlen(titulo) + 1);
    if (pelicula->titulo == NULL) {
        free(pelicula);
        return NULL;
    }
    strcpy(pelicula->titulo, titulo);
    
    pelicula->sala = sala;
    
    pelicula->horario = (char*)malloc(strlen(horario) + 1);
    if (pelicula->horario == NULL) {
        free(pelicula->titulo);
        free(pelicula);
        return NULL;
    }
    strcpy(pelicula->horario, horario);
    
    pelicula->filas = filas;
    pelicula->columnas = columnas;
    
    // Crear matriz dinámica de asientos
    pelicula->asientos = (int**)malloc(filas * sizeof(int*));
    if (pelicula->asientos == NULL) {
        free(pelicula->horario);
        free(pelicula->titulo);
        free(pelicula);
        return NULL;
    }
    
    for (int i = 0; i < filas; i++) {
        pelicula->asientos[i] = (int*)calloc(columnas, sizeof(int));
        if (pelicula->asientos[i] == NULL) {
            // Liberar lo ya creado
            for (int j = 0; j < i; j++) {
                free(pelicula->asientos[j]);
            }
            free(pelicula->asientos);
            free(pelicula->horario);
            free(pelicula->titulo);
            free(pelicula);
            return NULL;
        }
    }
    
    return pelicula;
}

void pelicula_liberar(Pelicula* pelicula) {
    if (pelicula != NULL) {
        if (pelicula->asientos != NULL) {
            for (int i = 0; i < pelicula->filas; i++) {
                if (pelicula->asientos[i] != NULL) {
                    free(pelicula->asientos[i]);
                }
            }
            free(pelicula->asientos);
        }
        if (pelicula->horario != NULL) {
            free(pelicula->horario);
        }
        if (pelicula->titulo != NULL) {
            free(pelicula->titulo);
        }
        free(pelicula);
    }
}

int pelicula_reservarAsiento(Pelicula* pelicula, int fila, int columna) {
    if (pelicula == NULL) return 0;
    if (fila < 0 || fila >= pelicula->filas) return 0;
    if (columna < 0 || columna >= pelicula->columnas) return 0;
    if (pelicula->asientos[fila][columna] == 1) return 0;
    
    pelicula->asientos[fila][columna] = 1;
    return 1;
}

int pelicula_liberarAsiento(Pelicula* pelicula, int fila, int columna) {
    if (pelicula == NULL) return 0;
    if (fila < 0 || fila >= pelicula->filas) return 0;
    if (columna < 0 || columna >= pelicula->columnas) return 0;
    if (pelicula->asientos[fila][columna] == 0) return 0;
    
    pelicula->asientos[fila][columna] = 0;
    return 1;
}

int pelicula_asientoOcupado(const Pelicula* pelicula, int fila, int columna) {
    if (pelicula == NULL) return 0;
    if (fila < 0 || fila >= pelicula->filas) return 0;
    if (columna < 0 || columna >= pelicula->columnas) return 0;
    return pelicula->asientos[fila][columna];
}

void pelicula_mostrarSala(const Pelicula* pelicula, char* buffer, int size) {
    if (pelicula == NULL || buffer == NULL) return;
    
    int offset = 0;
    offset += snprintf(buffer + offset, size - offset, 
                       "=== SALA %d - %s (%s) ===\n", 
                       pelicula->sala, pelicula->titulo, pelicula->horario);
    offset += snprintf(buffer + offset, size - offset,
                       "Filas: %d, Columnas: %d (L=Libre, O=Ocupado)\n",
                       pelicula->filas, pelicula->columnas);
    
    // Encabezado de columnas
    offset += snprintf(buffer + offset, size - offset, "   ");
    for (int j = 0; j < pelicula->columnas; j++) {
        offset += snprintf(buffer + offset, size - offset, "%d ", j + 1);
    }
    offset += snprintf(buffer + offset, size - offset, "\n");
    
    // Filas
    for (int i = 0; i < pelicula->filas; i++) {
        offset += snprintf(buffer + offset, size - offset, "%d ", i + 1);
        for (int j = 0; j < pelicula->columnas; j++) {
            offset += snprintf(buffer + offset, size - offset, "%c ", 
                               pelicula->asientos[i][j] == 0 ? 'L' : 'O');
        }
        offset += snprintf(buffer + offset, size - offset, "\n");
    }
    offset += snprintf(buffer + offset, size - offset, "===================================\n");
}

int pelicula_getFilas(const Pelicula* pelicula) {
    return pelicula->filas;
}

int pelicula_getColumnas(const Pelicula* pelicula) {
    return pelicula->columnas;
}