#include "persistencia.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ejecutar_sql(sqlite3* db, const char* sql) {
    char* err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] Error: %s\n    SQL: %s\n", err, sql);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

static int crear_tablas(sqlite3* db) {
    if (!ejecutar_sql(db,
        "CREATE TABLE IF NOT EXISTS tiendas ("
        "  id     INTEGER PRIMARY KEY,"
        "  nombre TEXT NOT NULL"
        ");"))
        return 0;

    if (!ejecutar_sql(db,
        "CREATE TABLE IF NOT EXISTS productos ("
        "  id        INTEGER PRIMARY KEY,"
        "  id_tienda INTEGER NOT NULL,"
        "  nombre    TEXT    NOT NULL,"
        "  precio    REAL    NOT NULL,"
        "  stock     INTEGER NOT NULL,"
        "  FOREIGN KEY(id_tienda) REFERENCES tiendas(id)"
        ");"))
        return 0;

    if (!ejecutar_sql(db,
        "CREATE TABLE IF NOT EXISTS peliculas ("
        "  id       INTEGER PRIMARY KEY,"
        "  titulo   TEXT    NOT NULL,"
        "  sala     INTEGER NOT NULL,"
        "  horario  TEXT    NOT NULL,"
        "  filas    INTEGER NOT NULL,"
        "  columnas INTEGER NOT NULL"
        ");"))
        return 0;

    if (!ejecutar_sql(db,
        "CREATE TABLE IF NOT EXISTS reservas ("
        "  id_pelicula INTEGER NOT NULL,"
        "  fila        INTEGER NOT NULL,"
        "  columna     INTEGER NOT NULL,"
        "  PRIMARY KEY(id_pelicula, fila, columna),"
        "  FOREIGN KEY(id_pelicula) REFERENCES peliculas(id)"
        ");"))
        return 0;

    if (!ejecutar_sql(db,
        "CREATE TABLE IF NOT EXISTS usuarios ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  password TEXT NOT NULL,"
        "  nombre   TEXT NOT NULL"
        ");"))
        return 0;

    return 1;
}


int guardar_datos(const CentroComercial* cc, const char* filename) {
    sqlite3* db = NULL;

    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "[DB] No se pudo abrir '%s': %s\n",
                filename, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    ejecutar_sql(db, "PRAGMA foreign_keys = ON;");
    ejecutar_sql(db, "PRAGMA journal_mode = WAL;");

    if (!crear_tablas(db)) {
        sqlite3_close(db);
        return 0;
    }

    if (!ejecutar_sql(db, "BEGIN TRANSACTION;")) {
        sqlite3_close(db);
        return 0;
    }

    ejecutar_sql(db, "DELETE FROM reservas;");
    ejecutar_sql(db, "DELETE FROM productos;");
    ejecutar_sql(db, "DELETE FROM peliculas;");
    ejecutar_sql(db, "DELETE FROM tiendas;");

    /* ---------- Tiendas ------------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "INSERT INTO tiendas (id, nombre) VALUES (?, ?);",
            -1, &stmt, NULL);

        for (int i = 0; i < cc->numTiendas; i++) {
            Tienda* t = cc->listaTiendas[i];
            sqlite3_bind_int (stmt, 1, t->id);
            sqlite3_bind_text(stmt, 2, t->nombre, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_DONE)
                fprintf(stderr, "[DB] Error INSERT tienda %d: %s\n",
                        t->id, sqlite3_errmsg(db));
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Productos ----------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "INSERT INTO productos (id, id_tienda, nombre, precio, stock) "
            "VALUES (?, ?, ?, ?, ?);",
            -1, &stmt, NULL);

        for (int i = 0; i < cc->numTiendas; i++) {
            Tienda* t = cc->listaTiendas[i];
            for (int j = 0; j < t->numProductos; j++) {
                Producto* p = t->inventario[j];
                sqlite3_bind_int   (stmt, 1, p->id);
                sqlite3_bind_int   (stmt, 2, t->id);
                sqlite3_bind_text  (stmt, 3, p->nombre, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 4, (double)p->precio);
                sqlite3_bind_int   (stmt, 5, p->stock);
                if (sqlite3_step(stmt) != SQLITE_DONE)
                    fprintf(stderr, "[DB] Error INSERT producto %d: %s\n",
                            p->id, sqlite3_errmsg(db));
                sqlite3_reset(stmt);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Peliculas ----------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "INSERT INTO peliculas (id, titulo, sala, horario, filas, columnas) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            -1, &stmt, NULL);

        for (int i = 0; i < cc->numPeliculas; i++) {
            Pelicula* p = cc->cartelera[i];
            sqlite3_bind_int (stmt, 1, p->id);
            sqlite3_bind_text(stmt, 2, p->titulo,  -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 3, p->sala);
            sqlite3_bind_text(stmt, 4, p->horario, -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 5, p->filas);
            sqlite3_bind_int (stmt, 6, p->columnas);
            if (sqlite3_step(stmt) != SQLITE_DONE)
                fprintf(stderr, "[DB] Error INSERT pelicula %d: %s\n",
                        p->id, sqlite3_errmsg(db));
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Reservas ------------------------------------------ */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "INSERT INTO reservas (id_pelicula, fila, columna) VALUES (?, ?, ?);",
            -1, &stmt, NULL);

        for (int i = 0; i < cc->numPeliculas; i++) {
            Pelicula* p = cc->cartelera[i];
            for (int f = 0; f < p->filas; f++) {
                for (int c = 0; c < p->columnas; c++) {
                    if (p->asientos[f][c] == 1) {
                        sqlite3_bind_int(stmt, 1, p->id);
                        sqlite3_bind_int(stmt, 2, f);
                        sqlite3_bind_int(stmt, 3, c);
                        if (sqlite3_step(stmt) != SQLITE_DONE)
                            fprintf(stderr, "[DB] Error INSERT reserva: %s\n",
                                    sqlite3_errmsg(db));
                        sqlite3_reset(stmt);
                    }
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    if (!ejecutar_sql(db, "COMMIT;")) {
        ejecutar_sql(db, "ROLLBACK;");
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);
    printf("[DB] Datos guardados en '%s'.\n", filename);
    return 1;
}

/* ================================================================== */
/*  cargar_datos                                                        */
/* ================================================================== */
int cargar_datos(CentroComercial* cc, const char* filename) {
    sqlite3* db = NULL;

    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "[DB] No se pudo abrir '%s': %s\n",
                filename, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    ejecutar_sql(db, "PRAGMA foreign_keys = ON;");

    /* Crear tablas si la BD es nueva */
    if (!crear_tablas(db)) {
        sqlite3_close(db);
        return 0;
    }

    /* Comprobar si hay datos reales */
    {
        sqlite3_stmt* check = NULL;
        int total = 0;

        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tiendas;", -1, &check, NULL);
        if (sqlite3_step(check) == SQLITE_ROW)
            total += sqlite3_column_int(check, 0);
        sqlite3_finalize(check);

        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM peliculas;", -1, &check, NULL);
        if (sqlite3_step(check) == SQLITE_ROW)
            total += sqlite3_column_int(check, 0);
        sqlite3_finalize(check);

        if (total == 0) {
            sqlite3_close(db);
            return 0;  /* BD vacía = primera ejecución */
        }
    }

    /* ---------- Cargar tiendas ------------------------------------ */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "SELECT id, nombre FROM tiendas ORDER BY id;",
            -1, &stmt, NULL);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int         id     = sqlite3_column_int (stmt, 0);
            const char* nombre = (const char*)sqlite3_column_text(stmt, 1);
            Tienda* t = tienda_crear(id, nombre);
            if (t == NULL || !cc_agregarTienda(cc, t)) {
                fprintf(stderr, "[DB] Error cargando tienda %d\n", id);
                if (t) tienda_liberar(t);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Cargar productos ---------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "SELECT id, id_tienda, nombre, precio, stock "
            "FROM productos ORDER BY id_tienda, id;",
            -1, &stmt, NULL);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int         id        = sqlite3_column_int   (stmt, 0);
            int         id_tienda = sqlite3_column_int   (stmt, 1);
            const char* nombre    = (const char*)sqlite3_column_text(stmt, 2);
            float       precio    = (float)sqlite3_column_double(stmt, 3);
            int         stock     = sqlite3_column_int   (stmt, 4);

            Tienda* t = cc_buscarTiendaPorId(cc, id_tienda);
            if (t == NULL) {
                fprintf(stderr, "[DB] Tienda %d no encontrada para producto %d\n",
                        id_tienda, id);
                continue;
            }
            Producto* p = producto_crear(id, nombre, precio, stock);
            if (p == NULL || !tienda_aniadirProducto(t, p)) {
                fprintf(stderr, "[DB] Error cargando producto %d\n", id);
                if (p) producto_liberar(p);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Cargar peliculas ---------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "SELECT id, titulo, sala, horario, filas, columnas "
            "FROM peliculas ORDER BY id;",
            -1, &stmt, NULL);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int         id       = sqlite3_column_int (stmt, 0);
            const char* titulo   = (const char*)sqlite3_column_text(stmt, 1);
            int         sala     = sqlite3_column_int (stmt, 2);
            const char* horario  = (const char*)sqlite3_column_text(stmt, 3);
            int         filas    = sqlite3_column_int (stmt, 4);
            int         columnas = sqlite3_column_int (stmt, 5);

            Pelicula* p = pelicula_crear(id, titulo, sala, horario, filas, columnas);
            if (p == NULL || !cc_agregarPelicula(cc, p)) {
                fprintf(stderr, "[DB] Error cargando pelicula %d\n", id);
                if (p) pelicula_liberar(p);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---------- Cargar reservas ----------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(db,
            "SELECT id_pelicula, fila, columna FROM reservas;",
            -1, &stmt, NULL);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id_peli = sqlite3_column_int(stmt, 0);
            int fila    = sqlite3_column_int(stmt, 1);
            int columna = sqlite3_column_int(stmt, 2);

            Pelicula* p = cc_buscarPeliculaPorId(cc, id_peli);
            if (p != NULL)
                pelicula_reservarAsiento(p, fila, columna);
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return 1;
}