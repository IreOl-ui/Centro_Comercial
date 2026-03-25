#ifndef TIENDA_H
#define TIENDA_H

#include "producto.h"

typedef struct Tienda {
    int id;
    char* nombre;
    Producto** inventario;  // Array dinámico de punteros a productos
    int numProductos;
} Tienda;

Tienda* tienda_crear(int id, const char* nombre);
// Crea una nueva tienda
// Devuelve un puntero a la tienda creada, NULL si falla

void tienda_liberar(Tienda* tienda);
// Libera toda la memoria asociada a la tienda y su inventario

int tienda_aniadirProducto(Tienda* tienda, Producto* producto);
// Añade un producto al inventario de la tienda
// Devuelve 1, y 0 si falla


int tienda_eliminarProducto(Tienda* tienda, int idProducto);
// Elimina un producto del inventario por su ID
// Devueleve 1, y 0 si no se encuentra el producto


Producto* tienda_buscarProductoPorId(const Tienda* tienda, int idProducto);
// Busca un producto en el inventario por su ID
// Devuelve un puntero al producto, y NULL si no existe


int tienda_modificarProducto(Tienda* tienda, int idProducto, float nuevoPrecio, int nuevoStock);
// Modifica el precio y stock de un producto
// Devuelve 1, y 0 si falla


int tienda_getNumProductos(const Tienda* tienda);
// Obtiene el número de productos en la tienda
// Devuelve la cantidad de productos


const char* tienda_getNombre(const Tienda* tienda);
// Obtiene el nombre de la tienda
// Devuelve el nombre de la tienda

#endif