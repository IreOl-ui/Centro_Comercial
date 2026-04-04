/*
 * persistencia.c  —  Implementación con SQLite3
 *
 * Compilar junto al resto del proyecto añadiendo sqlite3.c:
 *
 *   gcc main.c centro_comercial.c tienda.c producto.c pelicula.c \
 *       menu.c persistencia.c sqlite3.c -o deusto_centro
 *
 * (no hace falta -lsqlite3 porque se compila el amalgam directamente)
 */

#include "persistencia.h"
#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Helpers internos                                                    */
/* ------------------------------------------------------------------ */

/* Ejecuta SQL sin resultados dentro de una transacción ya abierta.
 * Devuelve 1 si ok, 0 si error (imprime el mensaje). */
static int ejecutar_sql(sqlite3* db, const char* sql) {
    char* err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[persistencia] Error SQL: %s\n  → %s\n", err, sql);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

/* Crea las cuatro tablas si no existen todavía. */
static int crear_tablas(sqlite3* db) {
    const char* ddl =
        "CREATE TABLE IF NOT EXISTS tiendas ("
        "  id      INTEGER PRIMARY KEY,"
        "  nombre  TEXT    NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS productos ("
        "  id         INTEGER PRIMARY KEY,"
        "  id_tienda  INTEGER NOT NULL REFERENCES tiendas(id),"
        "  nombre     TEXT    NOT NULL,"
        "  precio     REAL    NOT NULL,"
        "  stock      INTEGER NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS peliculas ("
        "  id       INTEGER PRIMARY KEY,"
        "  titulo   TEXT    NOT NULL,"
        "  sala     INTEGER NOT NULL,"
        "  horario  TEXT    NOT NULL,"
        "  filas    INTEGER NOT NULL,"
        "  columnas INTEGER NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS reservas ("
        "  id_pelicula  INTEGER NOT NULL REFERENCES peliculas(id),"
        "  fila         INTEGER NOT NULL,"
        "  columna      INTEGER NOT NULL,"
        "  PRIMARY KEY (id_pelicula, fila, columna)"
        ");";

    return ejecutar_sql(db, ddl);
}

/* ------------------------------------------------------------------ */
/*  cargar_datos                                                        */
/* ------------------------------------------------------------------ */

int cargar_datos(CentroComercial* cc, const char* filename) {
    sqlite3* db = NULL;

    /* Intentar abrir; si el fichero no existe sqlite3_open lo crea vacío */
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "[persistencia] No se pudo abrir '%s': %s\n",
                filename, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    /* Activar claves foráneas y crear tablas si hacen falta */
    ejecutar_sql(db, "PRAGMA foreign_keys = ON;");
    if (!crear_tablas(db)) {
        sqlite3_close(db);
        return 0;
    }

    /* ---- Cargar tiendas ------------------------------------------ */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql = "SELECT id, nombre FROM tiendas ORDER BY id;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare tiendas: %s\n",
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            return 0;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int    id     = sqlite3_column_int (stmt, 0);
            const char* nombre = (const char*)sqlite3_column_text(stmt, 1);

            Tienda* t = tienda_crear(id, nombre);
            if (t == NULL || !cc_agregarTienda(cc, t)) {
                if (t) tienda_liberar(t);
                fprintf(stderr, "[persistencia] Error al crear tienda id=%d\n", id);
                continue;
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Cargar productos ----------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql =
            "SELECT id, id_tienda, nombre, precio, stock "
            "FROM productos ORDER BY id_tienda, id;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare productos: %s\n",
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            return 0;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int   id        = sqlite3_column_int  (stmt, 0);
            int   id_tienda = sqlite3_column_int  (stmt, 1);
            const char* nombre = (const char*)sqlite3_column_text(stmt, 2);
            float precio    = (float)sqlite3_column_double(stmt, 3);
            int   stock     = sqlite3_column_int  (stmt, 4);

            Tienda* t = cc_buscarTiendaPorId(cc, id_tienda);
            if (t == NULL) {
                fprintf(stderr, "[persistencia] Tienda %d no encontrada para producto %d\n",
                        id_tienda, id);
                continue;
            }

            Producto* p = producto_crear(id, nombre, precio, stock);
            if (p == NULL || !tienda_aniadirProducto(t, p)) {
                if (p) producto_liberar(p);
                fprintf(stderr, "[persistencia] Error al crear producto id=%d\n", id);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Cargar películas ----------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql =
            "SELECT id, titulo, sala, horario, filas, columnas "
            "FROM peliculas ORDER BY id;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare peliculas: %s\n",
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            return 0;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int   id      = sqlite3_column_int (stmt, 0);
            const char* titulo  = (const char*)sqlite3_column_text(stmt, 1);
            int   sala    = sqlite3_column_int (stmt, 2);
            const char* horario = (const char*)sqlite3_column_text(stmt, 3);
            int   filas   = sqlite3_column_int (stmt, 4);
            int   columnas= sqlite3_column_int (stmt, 5);

            Pelicula* p = pelicula_crear(id, titulo, sala, horario, filas, columnas);
            if (p == NULL || !cc_agregarPelicula(cc, p)) {
                if (p) pelicula_liberar(p);
                fprintf(stderr, "[persistencia] Error al crear pelicula id=%d\n", id);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Cargar reservas ------------------------------------------ */
    {
        sqlite3_stmt* stmt = NULL;
        /* Las filas/columnas se guardaron ya en 0-indexado */
        const char* sql =
            "SELECT id_pelicula, fila, columna FROM reservas;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare reservas: %s\n",
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            return 0;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id_peli = sqlite3_column_int(stmt, 0);
            int fila    = sqlite3_column_int(stmt, 1);
            int columna = sqlite3_column_int(stmt, 2);

            Pelicula* p = cc_buscarPeliculaPorId(cc, id_peli);
            if (p != NULL) {
                pelicula_reservarAsiento(p, fila, columna);
            }
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  guardar_datos                                                       */
/* ------------------------------------------------------------------ */

int guardar_datos(const CentroComercial* cc, const char* filename) {
    sqlite3* db = NULL;

    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "[persistencia] No se pudo abrir '%s' para escritura: %s\n",
                filename, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    ejecutar_sql(db, "PRAGMA foreign_keys = ON;");
    if (!crear_tablas(db)) {
        sqlite3_close(db);
        return 0;
    }

    /* Todo dentro de una única transacción para atomicidad y velocidad */
    if (!ejecutar_sql(db, "BEGIN TRANSACTION;")) {
        sqlite3_close(db);
        return 0;
    }

    /* Limpiar datos anteriores (orden inverso por FK) */
    ejecutar_sql(db, "DELETE FROM reservas;");
    ejecutar_sql(db, "DELETE FROM productos;");
    ejecutar_sql(db, "DELETE FROM peliculas;");
    ejecutar_sql(db, "DELETE FROM tiendas;");

    /* ---- Guardar tiendas ------------------------------------------ */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql =
            "INSERT INTO tiendas (id, nombre) VALUES (?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare INSERT tiendas: %s\n",
                    sqlite3_errmsg(db));
            ejecutar_sql(db, "ROLLBACK;");
            sqlite3_close(db);
            return 0;
        }

        for (int i = 0; i < cc->numTiendas; i++) {
            Tienda* t = cc->listaTiendas[i];
            sqlite3_bind_int (stmt, 1, t->id);
            sqlite3_bind_text(stmt, 2, t->nombre, -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                fprintf(stderr, "[persistencia] INSERT tienda id=%d: %s\n",
                        t->id, sqlite3_errmsg(db));
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Guardar productos ---------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql =
            "INSERT INTO productos (id, id_tienda, nombre, precio, stock) "
            "VALUES (?, ?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare INSERT productos: %s\n",
                    sqlite3_errmsg(db));
            ejecutar_sql(db, "ROLLBACK;");
            sqlite3_close(db);
            return 0;
        }

        for (int i = 0; i < cc->numTiendas; i++) {
            Tienda* t = cc->listaTiendas[i];
            for (int j = 0; j < t->numProductos; j++) {
                Producto* p = t->inventario[j];
                sqlite3_bind_int   (stmt, 1, p->id);
                sqlite3_bind_int   (stmt, 2, t->id);
                sqlite3_bind_text  (stmt, 3, p->nombre, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 4, (double)p->precio);
                sqlite3_bind_int   (stmt, 5, p->stock);

                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    fprintf(stderr, "[persistencia] INSERT producto id=%d: %s\n",
                            p->id, sqlite3_errmsg(db));
                }
                sqlite3_reset(stmt);
            }
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Guardar películas ---------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        const char* sql =
            "INSERT INTO peliculas (id, titulo, sala, horario, filas, columnas) "
            "VALUES (?, ?, ?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare INSERT peliculas: %s\n",
                    sqlite3_errmsg(db));
            ejecutar_sql(db, "ROLLBACK;");
            sqlite3_close(db);
            return 0;
        }

        for (int i = 0; i < cc->numPeliculas; i++) {
            Pelicula* p = cc->cartelera[i];
            sqlite3_bind_int (stmt, 1, p->id);
            sqlite3_bind_text(stmt, 2, p->titulo,  -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 3, p->sala);
            sqlite3_bind_text(stmt, 4, p->horario, -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 5, p->filas);
            sqlite3_bind_int (stmt, 6, p->columnas);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                fprintf(stderr, "[persistencia] INSERT pelicula id=%d: %s\n",
                        p->id, sqlite3_errmsg(db));
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    /* ---- Guardar reservas ----------------------------------------- */
    {
        sqlite3_stmt* stmt = NULL;
        /* Guardamos en 0-indexado (coherente con pelicula_reservarAsiento) */
        const char* sql =
            "INSERT INTO reservas (id_pelicula, fila, columna) VALUES (?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "[persistencia] prepare INSERT reservas: %s\n",
                    sqlite3_errmsg(db));
            ejecutar_sql(db, "ROLLBACK;");
            sqlite3_close(db);
            return 0;
        }

        for (int i = 0; i < cc->numPeliculas; i++) {
            Pelicula* p = cc->cartelera[i];
            for (int f = 0; f < p->filas; f++) {
                for (int c = 0; c < p->columnas; c++) {
                    if (p->asientos[f][c] == 1) {
                        sqlite3_bind_int(stmt, 1, p->id);
                        sqlite3_bind_int(stmt, 2, f);   /* 0-indexado */
                        sqlite3_bind_int(stmt, 3, c);

                        if (sqlite3_step(stmt) != SQLITE_DONE) {
                            fprintf(stderr,
                                    "[persistencia] INSERT reserva peli=%d f=%d c=%d: %s\n",
                                    p->id, f, c, sqlite3_errmsg(db));
                        }
                        sqlite3_reset(stmt);
                    }
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    /* Confirmar transacción */
    if (!ejecutar_sql(db, "COMMIT;")) {
        ejecutar_sql(db, "ROLLBACK;");
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);
    return 1;
}