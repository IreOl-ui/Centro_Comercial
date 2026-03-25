#include "producto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Producto* producto_crear(int id, const char* nombre, float precio, int stock) {
    Producto* producto = (Producto*)malloc(sizeof(Producto));
    if (producto == NULL) {
        return NULL;
    }
    
    producto->id = id;
    producto->nombre = (char*)malloc(strlen(nombre) + 1);
    if (producto->nombre == NULL) {
        free(producto);
        return NULL;
    }
    strcpy(producto->nombre, nombre);
    
    producto->precio = precio;
    producto->stock = stock;
    
    return producto;
}

void producto_liberar(Producto* producto) {
    if (producto != NULL) {
        if (producto->nombre != NULL) {
            free(producto->nombre);
        }
        free(producto);
    }
}

void producto_toString(const Producto* producto, char* buffer, int size) {
    snprintf(buffer, size, "%d,%s,%.2f,%d", 
             producto->id, producto->nombre, producto->precio, producto->stock);
}