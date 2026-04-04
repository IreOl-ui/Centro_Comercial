#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "centro_comercial.h"
#include "menu.h"
#include "persistencia.h"
#include "config.h"
#include "usuario.h"
#include "log.h"
#include "sqlite3.h"


const char* g_db_path = NULL;

/* Variable global del usuario en sesión, leída por menu.c para logs */
char g_usuario_sesion[50] = "sistema";

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
/*  Menú de sesión: Iniciar sesión / Registrarse / Salir               */
/* ------------------------------------------------------------------ */
static int menu_sesion(sqlite3* db, const Config* cfg, Usuario* usuario_out) {
    int opcion;

    do {
        printf("\n====================================================\n");
        printf("         SISTEMA DE GESTION DEUSTO-CENTRO\n");
        printf("====================================================\n");
        printf("1. Iniciar sesion\n");
        printf("2. Registrar nuevo usuario\n");
        printf("3. Salir\n");
        printf("====================================================\n");
        printf("Seleccione: ");
        scanf("%d", &opcion);
        while (getchar() != '\n');

        if (opcion == 1) {
            /* --- LOGIN --- */
            char user[50], pass[50];
            int intentos = 0;

            printf("\n--- INICIO DE SESION ---\n");
            while (intentos < cfg->max_intentos) {
                printf("Usuario    : ");
                fgets(user, sizeof(user), stdin);
                user[strcspn(user, "\r\n")] = '\0';

                printf("Contrasena : ");
                fgets(pass, sizeof(pass), stdin);
                pass[strcspn(pass, "\r\n")] = '\0';

                /* Primero comprobar credenciales de admin del config */
                if (strcmp(user, cfg->admin_user) == 0 &&
                    strcmp(pass, cfg->admin_pass) == 0) {
                    usuario_out->id = 0;
                    strncpy(usuario_out->username, user, 49);
                    strncpy(usuario_out->nombre,   "Administrador", 99);
                    printf("\nBienvenido, Administrador.\n");
                    log_escribir(user, "LOGIN OK (admin)");
                    return 1;
                }

                /* Luego comprobar usuarios registrados en BD */
                if (usuario_login(db, user, pass, usuario_out)) {
                    printf("\nBienvenido, %s.\n", usuario_out->nombre);
                    log_escribir(user, "LOGIN OK");
                    return 1;
                }

                intentos++;
                int rest = cfg->max_intentos - intentos;
                log_escribir(user, "LOGIN FALLIDO");
                if (rest > 0)
                    printf("Credenciales incorrectas. Intentos restantes: %d\n", rest);
            }
            printf("Demasiados intentos fallidos.\n");

        } else if (opcion == 2) {
            /* --- REGISTRO --- */
            char user[50], pass[50], nombre[100];

            printf("\n--- REGISTRO DE USUARIO ---\n");
            printf("Nombre completo : ");
            fgets(nombre, sizeof(nombre), stdin);
            nombre[strcspn(nombre, "\r\n")] = '\0';

            printf("Usuario         : ");
            fgets(user, sizeof(user), stdin);
            user[strcspn(user, "\r\n")] = '\0';

            if (usuario_existe(db, user)) {
                printf("Error: ese nombre de usuario ya esta en uso.\n");
                continue;
            }

            printf("Contrasena      : ");
            fgets(pass, sizeof(pass), stdin);
            pass[strcspn(pass, "\r\n")] = '\0';

            if (usuario_registrar(db, user, pass, nombre)) {
                printf("\nUsuario '%s' registrado correctamente.\n", user);
                log_escribir(user, "REGISTRO OK");
            } else {
                printf("Error al registrar el usuario.\n");
            }

        } else if (opcion == 3) {
            log_escribir("sistema", "PROGRAMA CERRADO");
            return 0;   /* salir del programa */
        } else {
            printf("Opcion no valida.\n");
        }

    } while (1);
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
    g_db_path = cfg.db_path;

    /* 2. Iniciar log */
    log_init(cfg.log_path);
    log_escribir("sistema", "PROGRAMA INICIADO");

    /* 3. Abrir BD y crear tabla de usuarios si no existe */
    sqlite3* db = NULL;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Error abriendo BD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        log_cerrar();
        return 1;
    }
    usuario_crear_tabla(db);

    /* 4. Menú de sesión */
    Usuario usuario_activo;
    memset(&usuario_activo, 0, sizeof(Usuario));

    if (!menu_sesion(db, &cfg, &usuario_activo)) {
        /* El usuario eligió Salir */
        sqlite3_close(db);
        log_cerrar();
        printf("\n====================================================\n");
        printf("       SISTEMA DEUSTO-CENTRO FINALIZADO\n");
        printf("====================================================\n");
        return 0;
    }

    /* Guardar usuario en sesión para que menu.c pueda logarlo */
    strncpy(g_usuario_sesion, usuario_activo.username, 49);

    /* 5. Inicializar centro comercial */
    CentroComercial* cc = cc_crear();
    if (cc == NULL) {
        fprintf(stderr, "Error critico inicializando centro comercial.\n");
        sqlite3_close(db);
        log_cerrar();
        return 1;
    }

    /* 6. Cargar datos (o insertar ejemplos si es la primera vez) */
    if (cargar_datos(cc, cfg.db_path) && cc->numTiendas > 0) {
        printf("Datos cargados desde '%s'.\n", cfg.db_path);
    } else {
        printf("Base de datos nueva. Cargando datos iniciales...\n");
        cargar_datos_iniciales(cc, cfg.db_path);
    }

    printf("\nPresione Enter para continuar...");
    getchar();

    /* 7. Menú principal */
    menu_principal(cc);

    /* 8. Limpieza */
    log_escribir(g_usuario_sesion, "SESION CERRADA");
    cc_liberar(cc);
    sqlite3_close(db);
    log_cerrar();

    printf("\n====================================================\n");
    printf("       SISTEMA DEUSTO-CENTRO FINALIZADO\n");
    printf("====================================================\n");

    return 0;
}