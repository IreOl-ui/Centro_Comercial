#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "centro_comercial.h"
#include "menu.h"
#include "persistencia.h"
#include "config.h"

/* Variable global leida por menu.c */
const char* g_db_path = NULL;

/* ------------------------------------------------------------------ */
/*  Datos iniciales de ejemplo                                          */
/* ------------------------------------------------------------------ */
static void cargar_datos_iniciales(CentroComercial* cc, const char* db_path) {
    printf("Cargando datos iniciales de ejemplo...\n");

    Tienda* t1 = tienda_crear(1, "Zara");
    tienda_aniadirProducto(t1, producto_crear(101, "Camiseta Basica",   19.99,  50));
    tienda_aniadirProducto(t1, producto_crear(102, "Pantalon Vaquero",  39.99,  30));
    tienda_aniadirProducto(t1, producto_crear(103, "Vestido Verano",    29.99,  25));
    tienda_aniadirProducto(t1, producto_crear(104, "Chaqueta Denim",    49.99,  20));
    cc_agregarTienda(cc, t1);

    Tienda* t2 = tienda_crear(2, "MediaMarkt");
    tienda_aniadirProducto(t2, producto_crear(201, "Auriculares BT",      59.99, 15));
    tienda_aniadirProducto(t2, producto_crear(202, "Teclado Mecanico",    89.99, 10));
    tienda_aniadirProducto(t2, producto_crear(203, "Raton Inalambrico",   34.99, 20));
    tienda_aniadirProducto(t2, producto_crear(204, "Monitor 24 pulgadas",199.99,  8));
    cc_agregarTienda(cc, t2);

    Tienda* t3 = tienda_crear(3, "Mercadona");
    tienda_aniadirProducto(t3, producto_crear(301, "Leche Entera 1L",  0.89, 200));
    tienda_aniadirProducto(t3, producto_crear(302, "Pan de Molde",      1.29, 150));
    tienda_aniadirProducto(t3, producto_crear(303, "Yogur Natural x4",  0.99, 120));
    tienda_aniadirProducto(t3, producto_crear(304, "Aceite Oliva 1L",   4.49,  80));
    cc_agregarTienda(cc, t3);

    Tienda* t4 = tienda_crear(4, "Nike");
    tienda_aniadirProducto(t4, producto_crear(401, "Zapatillas Air Max", 129.99, 40));
    tienda_aniadirProducto(t4, producto_crear(402, "Camiseta Dri-FIT",   34.99,  60));
    tienda_aniadirProducto(t4, producto_crear(403, "Mochila Sport",       49.99,  25));
    cc_agregarTienda(cc, t4);

    Tienda* t5 = tienda_crear(5, "Casa del Libro");
    tienda_aniadirProducto(t5, producto_crear(501, "El Quijote", 12.99, 30));
    tienda_aniadirProducto(t5, producto_crear(502, "Dune",       15.99, 20));
    tienda_aniadirProducto(t5, producto_crear(503, "Sapiens",    18.99, 25));
    cc_agregarTienda(cc, t5);

    Pelicula* p1 = pelicula_crear(1, "Dune: Parte Dos", 1, "17:00", 8, 10);
    pelicula_reservarAsiento(p1, 0, 0); pelicula_reservarAsiento(p1, 0, 1);
    pelicula_reservarAsiento(p1, 3, 5); pelicula_reservarAsiento(p1, 4, 4);
    cc_agregarPelicula(cc, p1);

    Pelicula* p2 = pelicula_crear(2, "Deadpool & Wolverine", 2, "19:30", 6, 8);
    pelicula_reservarAsiento(p2, 1, 2); pelicula_reservarAsiento(p2, 1, 3);
    pelicula_reservarAsiento(p2, 2, 2); pelicula_reservarAsiento(p2, 2, 3);
    cc_agregarPelicula(cc, p2);

    Pelicula* p3 = pelicula_crear(3, "Inside Out 2", 3, "16:00", 7, 9);
    pelicula_reservarAsiento(p3, 0, 0); pelicula_reservarAsiento(p3, 0, 1);
    pelicula_reservarAsiento(p3, 0, 2);
    cc_agregarPelicula(cc, p3);

    Pelicula* p4 = pelicula_crear(4, "Oppenheimer", 1, "21:00", 8, 10);
    pelicula_reservarAsiento(p4, 5, 3); pelicula_reservarAsiento(p4, 5, 4);
    pelicula_reservarAsiento(p4, 6, 3); pelicula_reservarAsiento(p4, 6, 4);
    pelicula_reservarAsiento(p4, 6, 5);
    cc_agregarPelicula(cc, p4);

    if (guardar_datos(cc, db_path))
        printf("Datos iniciales guardados correctamente.\n");
    else
        printf("Advertencia: no se pudieron guardar los datos iniciales.\n");
}

/* ------------------------------------------------------------------ */
/*  Autenticacion                                                        */
/* ------------------------------------------------------------------ */
static int autenticar(const Config* cfg) {
    char user[50], pass[50];
    int intentos = 0;

    printf("\n====================================================\n");
    printf("          LOGIN - ADMINISTRADOR DEUSTO-CENTRO\n");
    printf("====================================================\n");

    while (intentos < cfg->max_intentos) {
        printf("Usuario    : ");
        if (fgets(user, sizeof(user), stdin) == NULL) return 0;
        user[strcspn(user, "\r\n")] = '\0';

        printf("Contrasena : ");
        if (fgets(pass, sizeof(pass), stdin) == NULL) return 0;
        pass[strcspn(pass, "\r\n")] = '\0';

        if (strcmp(user, cfg->admin_user) == 0 &&
            strcmp(pass, cfg->admin_pass) == 0) {
            printf("\nAcceso concedido. Bienvenido, %s.\n", user);
            return 1;
        }

        intentos++;
        int restantes = cfg->max_intentos - intentos;
        if (restantes > 0)
            printf("Credenciales incorrectas. Intentos restantes: %d\n\n", restantes);
    }

    printf("\nAcceso denegado. Demasiados intentos fallidos.\n");
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main() {
    printf("\n====================================================\n");
    printf("       INICIANDO SISTEMA DEUSTO-CENTRO\n");
    printf("====================================================\n");

    /* 1. Leer configuracion */
    Config cfg;
    config_cargar(&cfg, CONFIG_FILE);
    printf("Configuracion cargada desde '%s'.\n", CONFIG_FILE);
    printf("Base de datos : %s\n", cfg.db_path);
    g_db_path = cfg.db_path;

    /* 2. Autenticacion */
    if (!autenticar(&cfg)) {
        printf("El programa se cerrara por seguridad.\n");
        return 1;
    }

    /* 3. Inicializar centro comercial */
    CentroComercial* cc = cc_crear();
    if (cc == NULL) {
        printf("Error critico: No se pudo inicializar el sistema.\n");
        return 1;
    }

    /* 4. Cargar datos (o insertar ejemplos si es la primera vez) */
    if (cargar_datos(cc, cfg.db_path) && cc->numTiendas > 0) {
        printf("Datos cargados correctamente desde '%s'.\n", cfg.db_path);
    } else {
        printf("Base de datos nueva detectada.\n");
        cargar_datos_iniciales(cc, cfg.db_path);
    }

    printf("\nPresione Enter para continuar...");
    getchar();

    /* 5. Menu principal — pasamos db_path para que menu pueda guardar */
    menu_principal(cc);

    /* 6. Liberar y salir */
    cc_liberar(cc);

    printf("\n====================================================\n");
    printf("       SISTEMA DEUSTO-CENTRO FINALIZADO\n");
    printf("====================================================\n");

    return 0;
}