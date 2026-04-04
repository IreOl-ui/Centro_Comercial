#include "menu.h"
#include "persistencia.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* g_db_path;
extern char g_usuario_sesion[50];

static void menu_log(const char* accion) {
    log_escribir(g_usuario_sesion, accion);
}

static void limpiar_pantalla() {
    system("cls"); 
}

static void pausar() {
    printf("\nPresione Enter para continuar...");
    while (getchar() != '\n');
    getchar();
}

static void mostrar_tiendas(const CentroComercial* cc) {
    printf("\n========== LISTADO DE TIENDAS ==========\n");
    if (cc->numTiendas == 0) {
        printf("No hay tiendas registradas.\n");
    } else {
        for (int i = 0; i < cc->numTiendas; i++) {
            Tienda* t = cc->listaTiendas[i];
            printf("ID: %d | Nombre: %s | Productos: %d\n", 
                   t->id, t->nombre, t->numProductos);
        }
    }
    printf("=========================================\n");
}

static void mostrar_inventario(const Tienda* tienda) {
    printf("\n=== INVENTARIO DE TIENDA: %s (ID:%d) ===\n", 
           tienda->nombre, tienda->id);
    if (tienda->numProductos == 0) {
        printf("No hay productos en esta tienda.\n");
    } else {
        printf("%-8s %-20s %-8s %s\n", "ID", "NOMBRE", "PRECIO", "STOCK");
        printf("-------------------------------------------------\n");
        for (int i = 0; i < tienda->numProductos; i++) {
            Producto* p = tienda->inventario[i];
            printf("%-8d %-20s %.2f EUR %d\n", 
                   p->id, p->nombre, p->precio, p->stock);
        }
    }
    printf("=================================================\n");
}

void menu_principal(CentroComercial* cc) {
    int opcion;
    
    do {
        limpiar_pantalla();
        printf("\n====================================================\n");
        printf("   === BIENVENIDO AL SISTEMA DE GESTION DEUSTO-CENTRO ===\n");
        printf("====================================================\n");
        printf("1. Gestionar Tiendas\n");
        printf("2. Gestionar Inventario de Tienda\n");
        printf("3. Gestionar Cine\n");
        printf("4. Guardar y Salir\n");
        printf("====================================================\n");
        printf("Seleccione una opcion: ");
        
        scanf("%d", &opcion);
        while (getchar() != '\n');
        
        switch (opcion) {
            case 1:
                menu_gestion_tiendas(cc);
                break;
            case 2:
                menu_gestion_inventario(cc);
                break;
            case 3:
                menu_gestion_cine(cc);
                break;
            case 4:
                if (guardar_datos(cc, g_db_path)) {
                    printf("\nDatos guardados correctamente.\n");
                } else {
                    printf("\nError al guardar los datos.\n");
                }
                pausar();
                break;
            default:
                printf("\nOpcion no valida.\n");
                pausar();
        }
    } while (opcion != 4);
}

void menu_gestion_tiendas(CentroComercial* cc) {
    int opcion;
    
    do {
        limpiar_pantalla();
        mostrar_tiendas(cc);
        printf("\n--- GESTION DE TIENDAS ---\n");
        printf("1. Anadir Tienda\n");
        printf("2. Eliminar Tienda\n");
        printf("3. Volver\n");
        printf("Seleccione: ");
        
        scanf("%d", &opcion);
        while (getchar() != '\n');
        
        switch (opcion) {
            case 1: {
                int id;
                char nombre[100];
                
                printf("\n--- ANADIR TIENDA ---\n");
                printf("ID de la Tienda: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                printf("Nombre de la Tienda: ");
                fgets(nombre, sizeof(nombre), stdin);
                nombre[strcspn(nombre, "\n")] = '\0';
                
                Tienda* tienda = tienda_crear(id, nombre);
                if (tienda != NULL && cc_agregarTienda(cc, tienda)) {
                    printf("\nTienda '%s' anadida con exito!\n", nombre);
                    { char _b[128]; snprintf(_b,128,"TIENDA ANYADIDA id=%d %s",id,nombre); menu_log(_b); }
                } else {
                    printf("\nError: Ya existe una tienda con ID %d o memoria insuficiente.\n", id);
                    if (tienda != NULL) tienda_liberar(tienda);
                }
                pausar();
                break;
            }
            case 2: {
                if (cc->numTiendas == 0) {
                    printf("\nNo hay tiendas para eliminar.\n");
                    pausar();
                    break;
                }
                
                int id;
                printf("\n--- ELIMINAR TIENDA ---\n");
                printf("ID de la Tienda a eliminar: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                if (cc_eliminarTienda(cc, id)) {
                    printf("\nTienda eliminada correctamente.\n");
                    { char _b[64]; snprintf(_b,64,"TIENDA ELIMINADA id=%d",id); menu_log(_b); }
                } else {
                    printf("\nError: No se encontro tienda con ID %d.\n", id);
                }
                pausar();
                break;
            }
            case 3:
                break;
            default:
                printf("\nOpcion no valida.\n");
                pausar();
        }
    } while (opcion != 3);
}

void menu_gestion_inventario(CentroComercial* cc) {
    if (cc->numTiendas == 0) {
        limpiar_pantalla();
        printf("\nNo hay tiendas registradas. Primero debe crear una tienda.\n");
        pausar();
        return;
    }
    
    int idTienda;
    mostrar_tiendas(cc);
    printf("\nID de la Tienda a gestionar: ");
    scanf("%d", &idTienda);
    while (getchar() != '\n');
    
    Tienda* tienda = cc_buscarTiendaPorId(cc, idTienda);
    if (tienda == NULL) {
        printf("\nError: No se encontro tienda con ID %d.\n", idTienda);
        pausar();
        return;
    }
    
    int opcion;
    do {
        limpiar_pantalla();
        mostrar_inventario(tienda);
        printf("\n--- GESTION DE INVENTARIO - %s ---\n", tienda->nombre);
        printf("1. Anadir Producto\n");
        printf("2. Eliminar Producto\n");
        printf("3. Modificar Producto\n");
        printf("4. Volver\n");
        printf("Seleccione: ");
        
        scanf("%d", &opcion);
        while (getchar() != '\n');
        
        switch (opcion) {
            case 1: {
                int id, stock;
                char nombre[100];
                float precio;
                
                printf("\n--- ANADIR PRODUCTO ---\n");
                printf("ID del Producto: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                printf("Nombre del Producto: ");
                fgets(nombre, sizeof(nombre), stdin);
                nombre[strcspn(nombre, "\n")] = '\0';
                
                printf("Precio: ");
                scanf("%f", &precio);
                while (getchar() != '\n');
                
                printf("Stock: ");
                scanf("%d", &stock);
                while (getchar() != '\n');
                
                Producto* producto = producto_crear(id, nombre, precio, stock);
                if (producto != NULL && tienda_aniadirProducto(tienda, producto)) {
                    printf("\nProducto '%s' anadido a la tienda '%s' con exito!\n", 
                           nombre, tienda->nombre);
                    { char _b[256]; snprintf(_b,256,"PRODUCTO ANYADIDO id=%d %s tienda=%s",id,nombre,tienda->nombre); menu_log(_b); }
                    guardar_datos(cc, g_db_path);
                } else {
                    printf("\nError: Ya existe un producto con ID %d en esta tienda.\n", id);
                    if (producto != NULL) producto_liberar(producto);
                }
                pausar();
                break;
            }
            case 2: {
                if (tienda->numProductos == 0) {
                    printf("\nNo hay productos para eliminar.\n");
                    pausar();
                    break;
                }
                
                int id;
                printf("\n--- ELIMINAR PRODUCTO ---\n");
                printf("ID del Producto a eliminar: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                if (tienda_eliminarProducto(tienda, id)) {
                    printf("\nProducto eliminado correctamente.\n");
                } else {
                    printf("\nError: No se encontro producto con ID %d.\n", id);
                }
                pausar();
                break;
            }
            case 3: {
                if (tienda->numProductos == 0) {
                    printf("\nNo hay productos para modificar.\n");
                    pausar();
                    break;
                }
                
                int id, nuevoStock;
                float nuevoPrecio;
                
                printf("\n--- MODIFICAR PRODUCTO ---\n");
                printf("ID del Producto a modificar: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                Producto* producto = tienda_buscarProductoPorId(tienda, id);
                if (producto == NULL) {
                    printf("\nError: No se encontro producto con ID %d.\n", id);
                    pausar();
                    break;
                }
                
                printf("Producto actual: %s | Precio: %.2f | Stock: %d\n", 
                       producto->nombre, producto->precio, producto->stock);
                printf("Nuevo Precio: ");
                scanf("%f", &nuevoPrecio);
                while (getchar() != '\n');
                printf("Nuevo Stock: ");
                scanf("%d", &nuevoStock);
                while (getchar() != '\n');
                
                if (tienda_modificarProducto(tienda, id, nuevoPrecio, nuevoStock)) {
                    printf("\nProducto modificado correctamente.\n");
                    { char _b[128]; snprintf(_b,128,"PRODUCTO MODIFICADO id=%d precio=%.2f stock=%d",id,nuevoPrecio,nuevoStock); menu_log(_b); }
                } else {
                    printf("\nError al modificar el producto.\n");
                }
                pausar();
                break;
            }
            case 4:
                break;
            default:
                printf("\nOpcion no valida.\n");
                pausar();
        }
    } while (opcion != 4);
}

void menu_gestion_cine(CentroComercial* cc) {
    int opcion;
    
    do {
        limpiar_pantalla();
        printf("\n========== CARTELERA ==========\n");
        if (cc->numPeliculas == 0) {
            printf("No hay peliculas en cartelera.\n");
        } else {
            for (int i = 0; i < cc->numPeliculas; i++) {
                Pelicula* p = cc->cartelera[i];
                printf("ID: %d | %s | Sala %d | %s | %dx%d\n", 
                       p->id, p->titulo, p->sala, p->horario, 
                       p->filas, p->columnas);
            }
        }
        printf("===============================\n");
        
        printf("\n--- GESTION DE CINE ---\n");
        printf("1. Anadir Pelicula\n");
        printf("2. Eliminar Pelicula\n");
        printf("3. Reservar Asiento\n");
        printf("4. Ver Sala\n");
        printf("5. Volver\n");
        printf("Seleccione: ");
        
        scanf("%d", &opcion);
        while (getchar() != '\n');
        
        switch (opcion) {
            case 1: {
                int id, sala, filas, columnas;
                char titulo[200], horario[20];
                
                printf("\n--- ANADIR PELICULA ---\n");
                printf("ID de la Pelicula: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                printf("Titulo: ");
                fgets(titulo, sizeof(titulo), stdin);
                titulo[strcspn(titulo, "\n")] = '\0';
                
                printf("Numero de Sala: ");
                scanf("%d", &sala);
                while (getchar() != '\n');
                
                printf("Horario (HH:MM): ");
                fgets(horario, sizeof(horario), stdin);
                horario[strcspn(horario, "\n")] = '\0';
                
                printf("Numero de Filas: ");
                scanf("%d", &filas);
                while (getchar() != '\n');
                
                printf("Numero de Columnas: ");
                scanf("%d", &columnas);
                while (getchar() != '\n');
                
                Pelicula* pelicula = pelicula_crear(id, titulo, sala, horario, filas, columnas);
                if (pelicula != NULL && cc_agregarPelicula(cc, pelicula)) {
                    printf("\nPelicula '%s' anadida a la cartelera con exito!\n", titulo);
                    { char _b[256]; snprintf(_b,256,"PELICULA ANYADIDA id=%d sala=%d %s",id,sala,titulo); menu_log(_b); }
                    guardar_datos(cc, g_db_path);
                } else {
                    printf("\nError: Ya existe una pelicula en la sala %d a las %s.\n", sala, horario);
                    if (pelicula != NULL) pelicula_liberar(pelicula);
                }
                pausar();
                break;
            }
            case 2: {
                if (cc->numPeliculas == 0) {
                    printf("\nNo hay peliculas para eliminar.\n");
                    pausar();
                    break;
                }
                
                int id;
                printf("\n--- ELIMINAR PELICULA ---\n");
                printf("ID de la Pelicula a eliminar: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                if (cc_eliminarPelicula(cc, id)) {
                    printf("\nPelicula eliminada correctamente.\n");
                    { char _b[64]; snprintf(_b,64,"PELICULA ELIMINADA id=%d",id); menu_log(_b); }
                } else {
                    printf("\nError: No se encontro pelicula con ID %d.\n", id);
                }
                pausar();
                break;
            }
            case 3: {
                if (cc->numPeliculas == 0) {
                    printf("\nNo hay peliculas para reservar.\n");
                    pausar();
                    break;
                }
                
                int id, fila, columna;
                printf("\n--- RESERVAR ASIENTO ---\n");
                printf("ID de la Pelicula: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                Pelicula* pelicula = cc_buscarPeliculaPorId(cc, id);
                if (pelicula == NULL) {
                    printf("\nError: No se encontro pelicula con ID %d.\n", id);
                    pausar();
                    break;
                }
                
                printf("Fila (1-%d): ", pelicula->filas);
                scanf("%d", &fila);
                while (getchar() != '\n');
                printf("Columna (1-%d): ", pelicula->columnas);
                scanf("%d", &columna);
                while (getchar() != '\n');
                
                if (pelicula_reservarAsiento(pelicula, fila - 1, columna - 1)) {
                    printf("\nAsiento (%d,%d) reservado correctamente.\n", fila, columna);
                    { char _b[128]; snprintf(_b,128,"ASIENTO RESERVADO pelicula=%d fila=%d col=%d",id,fila,columna); menu_log(_b); }
                } else {
                    printf("\nError: Asiento ya ocupado o coordenadas invalidas.\n");
                }
                pausar();
                break;
            }
            case 4: {
                if (cc->numPeliculas == 0) {
                    printf("\nNo hay peliculas para visualizar.\n");
                    pausar();
                    break;
                }
                
                int id;
                printf("\n--- VER SALA ---\n");
                printf("ID de la Pelicula: ");
                scanf("%d", &id);
                while (getchar() != '\n');
                
                Pelicula* pelicula = cc_buscarPeliculaPorId(cc, id);
                if (pelicula == NULL) {
                    printf("\nError: No se encontro pelicula con ID %d.\n", id);
                    pausar();
                    break;
                }
                
                char buffer[4096];
                pelicula_mostrarSala(pelicula, buffer, sizeof(buffer));
                printf("\n%s", buffer);
                pausar();
                break;
            }
            case 5:
                break;
            default:
                printf("\nOpcion no valida.\n");
                pausar();
        }
    } while (opcion != 5);
}