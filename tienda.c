#include "tienda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Tienda* tienda_crear(int id, const char* nombre) {
    Tienda* tienda = (Tienda*)malloc(sizeof(Tienda));
    if (tienda == NULL) {
        return NULL;
    }
    
    tienda->id = id;
    tienda->nombre = (char*)malloc(strlen(nombre) + 1);
    if (tienda->nombre == NULL) {
        free(tienda);
        return NULL;
    }
    strcpy(tienda->nombre, nombre);
    
    tienda->inventario = NULL;
    tienda->numProductos = 0;
    
    return tienda;
}

void tienda_liberar(Tienda* tienda) {
    if (tienda != NULL) {
        if (tienda->nombre != NULL) {
            free(tienda->nombre);
        }
        if (tienda->inventario != NULL) {
            for (int i = 0; i < tienda->numProductos; i++) {
                producto_liberar(tienda->inventario[i]);
            }
            free(tienda->inventario);
        }
        free(tienda);
    }
}

int tienda_aniadirProducto(Tienda* tienda, Producto* producto) {
    if (tienda == NULL || producto == NULL) {
        return 0;
    }
    
    // Verificar que el producto no existe ya
    if (tienda_buscarProductoPorId(tienda, producto->id) != NULL) {
        return 0;
    }
    
    // Reajustar el array de inventario
    Producto** nuevoInventario = (Producto**)realloc(tienda->inventario, 
                                        (tienda->numProductos + 1) * sizeof(Producto*));
    if (nuevoInventario == NULL) {
        return 0;
    }
    
    tienda->inventario = nuevoInventario;
    tienda->inventario[tienda->numProductos] = producto;
    tienda->numProductos++;
    
    return 1;
}

int tienda_eliminarProducto(Tienda* tienda, int idProducto) {
    if (tienda == NULL || tienda->inventario == NULL) {
        return 0;
    }
    
    for (int i = 0; i < tienda->numProductos; i++) {
        if (tienda->inventario[i]->id == idProducto) {
            // Liberar memoria del producto
            producto_liberar(tienda->inventario[i]);
            
            // Desplazar los elementos restantes
            for (int j = i; j < tienda->numProductos - 1; j++) {
                tienda->inventario[j] = tienda->inventario[j + 1];
            }
            
            tienda->numProductos--;
            
            // Reajusatar el array
            if (tienda->numProductos == 0) {
                free(tienda->inventario);
                tienda->inventario = NULL;
            } else {
                Producto** nuevoInventario = (Producto**)realloc(tienda->inventario, 
                                                tienda->numProductos * sizeof(Producto*));
                if (nuevoInventario != NULL) {
                    tienda->inventario = nuevoInventario;
                }
            }
            return 1;
        }
    }
    return 0;
}

Producto* tienda_buscarProductoPorId(const Tienda* tienda, int idProducto) {
    if (tienda == NULL || tienda->inventario == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < tienda->numProductos; i++) {
        if (tienda->inventario[i]->id == idProducto) {
            return tienda->inventario[i];
        }
    }
    return NULL;
}

int tienda_modificarProducto(Tienda* tienda, int idProducto, float nuevoPrecio, int nuevoStock) {
    Producto* producto = tienda_buscarProductoPorId(tienda, idProducto);
    if (producto == NULL) {
        return 0;
    }
    
    producto->precio = nuevoPrecio;
    producto->stock = nuevoStock;
    return 1;
}

int tienda_getNumProductos(const Tienda* tienda) {
    return tienda->numProductos;
}

const char* tienda_getNombre(const Tienda* tienda) {
    return tienda->nombre;
}