#include "usuario.h"
#include <stdio.h>
#include <string.h>

int usuario_crear_tabla(sqlite3* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS usuarios ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username  TEXT    NOT NULL UNIQUE,"
        "  password  TEXT    NOT NULL,"
        "  nombre    TEXT    NOT NULL"
        ");";
    char* err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[usuario] Error creando tabla: %s\n", err);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

int usuario_existe(sqlite3* db, const char* username) {
    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT COUNT(*) FROM usuarios WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int existe = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        existe = sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return existe;
}

int usuario_registrar(sqlite3* db, const char* username,
                      const char* password, const char* nombre) {
    if (usuario_existe(db, username)) return 0;

    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "INSERT INTO usuarios (username, password, nombre) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, nombre,   -1, SQLITE_STATIC);
    int ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

int usuario_login(sqlite3* db, const char* username,
                  const char* password, Usuario* out) {
    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "SELECT id, username, password, nombre FROM usuarios "
        "WHERE username = ? AND password = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    int ok = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out->id = sqlite3_column_int(stmt, 0);
        strncpy(out->username, (const char*)sqlite3_column_text(stmt, 1), 49);
        strncpy(out->password, (const char*)sqlite3_column_text(stmt, 2), 49);
        strncpy(out->nombre,   (const char*)sqlite3_column_text(stmt, 3), 99);
        ok = 1;
    }
    sqlite3_finalize(stmt);
    return ok;
}