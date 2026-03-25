#ifndef PRODUCTO_H
#define PRODUCTO_H

typedef struct Producto {
    int id;
    char* nombre;
    float precio;
    int stock; // Cantidad disponible
} Producto;


Producto* producto_crear(int id, const char* nombre, float precio, int stock);
// Crea un nuevo producto en memoria dinámica
// Devuelve un puntero al producto creado, NULL si falla

void producto_liberar(Producto* producto);
// Libera la memoria asociada a un producto

void producto_toString(const Producto* producto, char* buffer, int size);

#endif