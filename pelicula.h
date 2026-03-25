#ifndef PELICULA_H
#define PELICULA_H

typedef struct Pelicula {
    int id;
    char* titulo;
    int sala;
    char* horario;
    int filas;
    int columnas;
    int** asientos;  // Matriz dinámica: 0=libre, 1=ocupado
} Pelicula;


Pelicula* pelicula_crear(int id, const char* titulo, int sala, const char* horario, 
                         int filas, int columnas);
// Crea una nueva película con su sala
// Devuelve un puntero a la película creada, NULL si falla


void pelicula_liberar(Pelicula* pelicula);
// Libera la memoria asociada a la película y su matriz de asientos


int pelicula_reservarAsiento(Pelicula* pelicula, int fila, int columna);
// Reserva un asiento específico
// Devuelve 1, y 0 si ya estaba ocupado o coordenadas no validas 


int pelicula_liberarAsiento(Pelicula* pelicula, int fila, int columna);
// Libera un asiento (cambia a libre)
// Devuelve 1, y 0 si ya estaba libre o coordenadas no validas 


int pelicula_asientoOcupado(const Pelicula* pelicula, int fila, int columna);
// Verifica si un asiento está ocupado
// Devuelve 1, y 0 si ya estaba libre o coordenadas no validas 


void pelicula_mostrarSala(const Pelicula* pelicula, char* buffer, int size);
// Obtiene el estado de la sala como texto
// Buffer para almacenar el estado como texto


int pelicula_getFilas(const Pelicula* pelicula);
// Obtiene el número de filas


int pelicula_getColumnas(const Pelicula* pelicula);
// Obtiene el número de columnas

#endif