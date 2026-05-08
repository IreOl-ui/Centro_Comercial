/*
 * servidor.c — Servidor del Centro Comercial Deusto (Hito 3)
 * Lenguaje: C
 * Sockets:  Winsock2 (Windows)
 *
 * Compilar (desde la carpeta del proyecto):
 *   gcc servidor.c centro_comercial.c tienda.c producto.c pelicula.c \
 *       persistencia.c usuario.c config.c log.c sqlite3.c \
 *       -o servidor.exe -lws2_32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "protocolo.h"
#include "centro_comercial.h"
#include "persistencia.h"
#include "usuario.h"
#include "config.h"
#include "log.h"
#include "sqlite3.h"

/* ------------------------------------------------------------------ */
/*  Globals                                                             */
/* ------------------------------------------------------------------ */
static CentroComercial* g_cc   = NULL;
static sqlite3*         g_db   = NULL;
static Config           g_cfg;

/* Necesario para que log.c y menu.c (si se reutiliza) funcionen */
const char* g_db_path          = NULL;
char        g_usuario_sesion[50] = "servidor";

/* ------------------------------------------------------------------ */
/*  Helpers de red                                                      */
/* ------------------------------------------------------------------ */

/* Envía una línea terminada en \n */
static void enviar(SOCKET sock, const char* msg) {
    char buf[PROTO_BUFSIZE];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    send(sock, buf, (int)strlen(buf), 0);
}

/* Recibe una línea (hasta \n o fin de buffer) */
static int recibir(SOCKET sock, char* buf, int size) {
    int total = 0;
    char c;
    while (total < size - 1) {
        int r = recv(sock, &c, 1, 0);
        if (r <= 0) return 0;
        if (c == '\n') break;
        if (c == '\r') continue;
        buf[total++] = c;
    }
    buf[total] = '\0';
    return total;
}

/* ------------------------------------------------------------------ */
/*  Manejador de peticiones                                             */
/* ------------------------------------------------------------------ */
static void manejar_cliente(SOCKET sock) {
    char buf[PROTO_BUFSIZE];
    char usuario_activo[50] = "";
    int  autenticado = 0;

    log_escribir("servidor", "Cliente conectado");

    while (1) {
        if (!recibir(sock, buf, sizeof(buf))) break;
        if (strlen(buf) == 0) continue;

        log_escribir("servidor", buf);

        /* Tokenizar */
        char copia[PROTO_BUFSIZE];
        strncpy(copia, buf, sizeof(copia));
        char* cmd = strtok(copia, PROTO_SEP);
        if (cmd == NULL) { enviar(sock, "ERR|Comando vacío"); continue; }

        /* ---- EXIT / LOGOUT ---------------------------------------- */
        if (strcmp(cmd, CMD_EXIT) == 0) {
            log_escribir(usuario_activo[0] ? usuario_activo : "?", "DESCONEXION");
            break;
        }
        if (strcmp(cmd, CMD_LOGOUT) == 0) {
            log_escribir(usuario_activo, "LOGOUT");
            autenticado = 0;
            usuario_activo[0] = '\0';
            enviar(sock, "OK|Sesión cerrada");
            continue;
        }

        /* ---- LOGIN ------------------------------------------------- */
        if (strcmp(cmd, CMD_LOGIN) == 0) {
            char* user = strtok(NULL, PROTO_SEP);
            char* pass = strtok(NULL, PROTO_SEP);
            if (!user || !pass) { enviar(sock, "ERR|Faltan parámetros"); continue; }

            Usuario u;
            int ok = 0;
            /* Comprobar admin del config */
            if (strcmp(user, g_cfg.admin_user) == 0 &&
                strcmp(pass, g_cfg.admin_pass) == 0) {
                ok = 1;
                strncpy(u.nombre, "Administrador", 99);
            } else {
                ok = usuario_login(g_db, user, pass, &u);
            }

            if (ok) {
                autenticado = 1;
                strncpy(usuario_activo, user, 49);
                char resp[128];
                snprintf(resp, sizeof(resp), "OK|Bienvenido %s", u.nombre);
                enviar(sock, resp);
                log_escribir(user, "LOGIN OK");
            } else {
                enviar(sock, "ERR|Credenciales incorrectas");
                log_escribir(user, "LOGIN FALLIDO");
            }
            continue;
        }

        /* Resto de comandos requieren autenticación */
        if (!autenticado) { enviar(sock, "ERR|No autenticado"); continue; }

        /* ---- GET_TIENDAS ------------------------------------------ */
        if (strcmp(cmd, CMD_GET_TIENDAS) == 0) {
            char resp[PROTO_BUFSIZE];
            int off = snprintf(resp, sizeof(resp), "OK");
            for (int i = 0; i < g_cc->numTiendas; i++) {
                Tienda* t = g_cc->listaTiendas[i];
                off += snprintf(resp + off, sizeof(resp) - off,
                                "|%d;%s;%d", t->id, t->nombre, t->numProductos);
            }
            enviar(sock, resp);
            continue;
        }

        /* ---- GET_PRODUCTOS ---------------------------------------- */
        if (strcmp(cmd, CMD_GET_PRODUCTOS) == 0) {
            char* sid = strtok(NULL, PROTO_SEP);
            if (!sid) { enviar(sock, "ERR|Falta id_tienda"); continue; }
            Tienda* t = cc_buscarTiendaPorId(g_cc, atoi(sid));
            if (!t) { enviar(sock, "ERR|Tienda no encontrada"); continue; }
            char resp[PROTO_BUFSIZE];
            int off = snprintf(resp, sizeof(resp), "OK");
            for (int i = 0; i < t->numProductos; i++) {
                Producto* p = t->inventario[i];
                off += snprintf(resp + off, sizeof(resp) - off,
                                "|%d;%s;%.2f;%d", p->id, p->nombre, p->precio, p->stock);
            }
            enviar(sock, resp);
            continue;
        }

        /* ---- ADD_TIENDA ------------------------------------------- */
        if (strcmp(cmd, CMD_ADD_TIENDA) == 0) {
            char* sid    = strtok(NULL, PROTO_SEP);
            char* nombre = strtok(NULL, PROTO_SEP);
            if (!sid || !nombre) { enviar(sock, "ERR|Faltan parámetros"); continue; }
            Tienda* t = tienda_crear(atoi(sid), nombre);
            if (t && cc_agregarTienda(g_cc, t)) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[64]; snprintf(msg, sizeof(msg), "TIENDA ANYADIDA id=%s", sid);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Tienda añadida");
            } else {
                if (t) tienda_liberar(t);
                enviar(sock, "ERR|Ya existe tienda con ese ID");
            }
            continue;
        }

        /* ---- DEL_TIENDA ------------------------------------------- */
        if (strcmp(cmd, CMD_DEL_TIENDA) == 0) {
            char* sid = strtok(NULL, PROTO_SEP);
            if (!sid) { enviar(sock, "ERR|Falta id"); continue; }
            if (cc_eliminarTienda(g_cc, atoi(sid))) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[64]; snprintf(msg, sizeof(msg), "TIENDA ELIMINADA id=%s", sid);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Tienda eliminada");
            } else {
                enviar(sock, "ERR|Tienda no encontrada");
            }
            continue;
        }

        /* ---- ADD_PRODUCTO ----------------------------------------- */
        if (strcmp(cmd, CMD_ADD_PRODUCTO) == 0) {
            char* sid       = strtok(NULL, PROTO_SEP);
            char* sid_tienda= strtok(NULL, PROTO_SEP);
            char* nombre    = strtok(NULL, PROTO_SEP);
            char* sprecio   = strtok(NULL, PROTO_SEP);
            char* sstock    = strtok(NULL, PROTO_SEP);
            if (!sid||!sid_tienda||!nombre||!sprecio||!sstock)
                { enviar(sock, "ERR|Faltan parámetros"); continue; }
            Tienda* t = cc_buscarTiendaPorId(g_cc, atoi(sid_tienda));
            if (!t) { enviar(sock, "ERR|Tienda no encontrada"); continue; }
            Producto* p = producto_crear(atoi(sid), nombre,
                                         (float)atof(sprecio), atoi(sstock));
            if (p && tienda_aniadirProducto(t, p)) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[128]; snprintf(msg,sizeof(msg),"PRODUCTO ANYADIDO id=%s %s",sid,nombre);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Producto añadido");
            } else {
                if (p) producto_liberar(p);
                enviar(sock, "ERR|Ya existe producto con ese ID");
            }
            continue;
        }

        /* ---- DEL_PRODUCTO ----------------------------------------- */
        if (strcmp(cmd, CMD_DEL_PRODUCTO) == 0) {
            char* sid_tienda  = strtok(NULL, PROTO_SEP);
            char* sid_producto = strtok(NULL, PROTO_SEP);
            if (!sid_tienda||!sid_producto) { enviar(sock,"ERR|Faltan parámetros"); continue; }
            Tienda* t = cc_buscarTiendaPorId(g_cc, atoi(sid_tienda));
            if (!t) { enviar(sock, "ERR|Tienda no encontrada"); continue; }
            if (tienda_eliminarProducto(t, atoi(sid_producto))) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[64]; snprintf(msg,sizeof(msg),"PRODUCTO ELIMINADO id=%s",sid_producto);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Producto eliminado");
            } else {
                enviar(sock, "ERR|Producto no encontrado");
            }
            continue;
        }

        /* ---- MOD_PRODUCTO ----------------------------------------- */
        if (strcmp(cmd, CMD_MOD_PRODUCTO) == 0) {
            char* sid_tienda  = strtok(NULL, PROTO_SEP);
            char* sid_producto= strtok(NULL, PROTO_SEP);
            char* sprecio     = strtok(NULL, PROTO_SEP);
            char* sstock      = strtok(NULL, PROTO_SEP);
            if (!sid_tienda||!sid_producto||!sprecio||!sstock)
                { enviar(sock,"ERR|Faltan parámetros"); continue; }
            Tienda* t = cc_buscarTiendaPorId(g_cc, atoi(sid_tienda));
            if (!t) { enviar(sock, "ERR|Tienda no encontrada"); continue; }
            if (tienda_modificarProducto(t, atoi(sid_producto),
                                         (float)atof(sprecio), atoi(sstock))) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[128]; snprintf(msg,sizeof(msg),
                    "PRODUCTO MODIFICADO id=%s precio=%s stock=%s",sid_producto,sprecio,sstock);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Producto modificado");
            } else {
                enviar(sock, "ERR|Producto no encontrado");
            }
            continue;
        }

        /* ---- GET_CARTELERA ---------------------------------------- */
        if (strcmp(cmd, CMD_GET_CARTELERA) == 0) {
            char resp[PROTO_BUFSIZE];
            int off = snprintf(resp, sizeof(resp), "OK");
            for (int i = 0; i < g_cc->numPeliculas; i++) {
                Pelicula* p = g_cc->cartelera[i];
                off += snprintf(resp + off, sizeof(resp) - off,
                                "|%d;%s;%d;%s;%d;%d",
                                p->id, p->titulo, p->sala,
                                p->horario, p->filas, p->columnas);
            }
            enviar(sock, resp);
            continue;
        }

        /* ---- ADD_PELICULA ----------------------------------------- */
        if (strcmp(cmd, CMD_ADD_PELICULA) == 0) {
            char* sid     = strtok(NULL, PROTO_SEP);
            char* titulo  = strtok(NULL, PROTO_SEP);
            char* ssala   = strtok(NULL, PROTO_SEP);
            char* horario = strtok(NULL, PROTO_SEP);
            char* sfilas  = strtok(NULL, PROTO_SEP);
            char* scols   = strtok(NULL, PROTO_SEP);
            if (!sid||!titulo||!ssala||!horario||!sfilas||!scols)
                { enviar(sock,"ERR|Faltan parámetros"); continue; }
            Pelicula* p = pelicula_crear(atoi(sid), titulo, atoi(ssala),
                                         horario, atoi(sfilas), atoi(scols));
            if (p && cc_agregarPelicula(g_cc, p)) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[128]; snprintf(msg,sizeof(msg),"PELICULA ANYADIDA id=%s %s",sid,titulo);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Película añadida");
            } else {
                if (p) pelicula_liberar(p);
                enviar(sock, "ERR|Sala y horario ya ocupados");
            }
            continue;
        }

        /* ---- DEL_PELICULA ----------------------------------------- */
        if (strcmp(cmd, CMD_DEL_PELICULA) == 0) {
            char* sid = strtok(NULL, PROTO_SEP);
            if (!sid) { enviar(sock, "ERR|Falta id"); continue; }
            if (cc_eliminarPelicula(g_cc, atoi(sid))) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[64]; snprintf(msg,sizeof(msg),"PELICULA ELIMINADA id=%s",sid);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Película eliminada");
            } else {
                enviar(sock, "ERR|Película no encontrada");
            }
            continue;
        }

        /* ---- GET_SALA --------------------------------------------- */
        if (strcmp(cmd, CMD_GET_SALA) == 0) {
            char* sid = strtok(NULL, PROTO_SEP);
            if (!sid) { enviar(sock, "ERR|Falta id"); continue; }
            Pelicula* p = cc_buscarPeliculaPorId(g_cc, atoi(sid));
            if (!p) { enviar(sock, "ERR|Película no encontrada"); continue; }
            /* Enviar filas x columnas de asientos: OK|filas|cols|0101... */
            char resp[PROTO_BUFSIZE];
            int off = snprintf(resp, sizeof(resp), "OK|%d|%d|",
                               p->filas, p->columnas);
            for (int f = 0; f < p->filas; f++)
                for (int c = 0; c < p->columnas; c++)
                    resp[off++] = p->asientos[f][c] ? '1' : '0';
            resp[off] = '\0';
            enviar(sock, resp);
            continue;
        }

        /* ---- RESERVAR --------------------------------------------- */
        if (strcmp(cmd, CMD_RESERVAR) == 0) {
            char* sid    = strtok(NULL, PROTO_SEP);
            char* sfila  = strtok(NULL, PROTO_SEP);
            char* scol   = strtok(NULL, PROTO_SEP);
            if (!sid||!sfila||!scol) { enviar(sock,"ERR|Faltan parámetros"); continue; }
            Pelicula* p = cc_buscarPeliculaPorId(g_cc, atoi(sid));
            if (!p) { enviar(sock, "ERR|Película no encontrada"); continue; }
            if (pelicula_reservarAsiento(p, atoi(sfila)-1, atoi(scol)-1)) {
                guardar_datos(g_cc, g_cfg.db_path);
                char msg[128]; snprintf(msg,sizeof(msg),
                    "ASIENTO RESERVADO pelicula=%s fila=%s col=%s",sid,sfila,scol);
                log_escribir(usuario_activo, msg);
                enviar(sock, "OK|Asiento reservado");
            } else {
                enviar(sock, "ERR|Asiento ya ocupado o coordenadas inválidas");
            }
            continue;
        }

        /* Comando desconocido */
        enviar(sock, "ERR|Comando desconocido");
    }

    closesocket(sock);
    log_escribir("servidor", "Cliente desconectado");
}

/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main() {
    printf("====================================================\n");
    printf("     SERVIDOR DEUSTO-CENTRO (Hito 3)\n");
    printf("====================================================\n");

    /* 1. Configuración */
    config_cargar(&g_cfg, CONFIG_FILE);
    g_db_path = g_cfg.db_path;
    log_init(g_cfg.log_path);
    log_escribir("sistema", "SERVIDOR INICIADO");

    /* 2. Base de datos */
    if (sqlite3_open(g_cfg.db_path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Error abriendo BD: %s\n", sqlite3_errmsg(g_db));
        return 1;
    }
    usuario_crear_tabla(g_db);

    /* 3. Centro comercial */
    g_cc = cc_crear();
    if (!g_cc) { fprintf(stderr, "Error creando CC\n"); return 1; }
    if (cargar_datos(g_cc, g_cfg.db_path))
        printf("Datos cargados desde '%s'.\n", g_cfg.db_path);
    else
        printf("BD vacía — inicia el admin local primero.\n");

    /* 4. Winsock */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "Error WSAStartup: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET) {
        fprintf(stderr, "Error creando socket: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }

    /* Permitir reusar el puerto */
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(g_cfg.server_port);

    if (bind(srv, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Error bind: %d\n", WSAGetLastError());
        closesocket(srv); WSACleanup(); return 1;
    }
    if (listen(srv, 1) == SOCKET_ERROR) {
        fprintf(stderr, "Error listen: %d\n", WSAGetLastError());
        closesocket(srv); WSACleanup(); return 1;
    }

    printf("Servidor escuchando en puerto %d...\n", g_cfg.server_port);
    printf("(Ctrl+C para detener)\n\n");
    log_escribir("sistema", "Escuchando conexiones");

    /* 5. Bucle principal — un cliente a la vez (sin hilos, como pide el PDF) */
    while (1) {
        SOCKADDR_IN cli_addr;
        int cli_len = sizeof(cli_addr);
        SOCKET cli = accept(srv, (SOCKADDR*)&cli_addr, &cli_len);
        if (cli == INVALID_SOCKET) {
            fprintf(stderr, "Error accept: %d\n", WSAGetLastError());
            continue;
        }

        char* ip = inet_ntoa(cli_addr.sin_addr);
        printf("[%s] Cliente conectado\n", ip);

        manejar_cliente(cli);

        printf("[%s] Cliente desconectado\n", ip);
    }

    /* Limpieza (inalcanzable salvo señal) */
    closesocket(srv);
    WSACleanup();
    cc_liberar(g_cc);
    sqlite3_close(g_db);
    log_cerrar();
    return 0;
}