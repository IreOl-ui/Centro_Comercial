#ifndef PROTOCOLO_H
#define PROTOCOLO_H

/*
 * protocolo.h — Protocolo de comunicación cliente/servidor
 *
 * Formato de mensajes: texto plano, una línea por mensaje.
 * Petición:  CMD|param1|param2|...
 * Respuesta: OK|datos...   o   ERR|mensaje
 *
 * Comandos:
 *   LOGIN|user|pass
 *   GET_TIENDAS
 *   GET_PRODUCTOS|id_tienda
 *   ADD_TIENDA|id|nombre
 *   DEL_TIENDA|id
 *   ADD_PRODUCTO|id|id_tienda|nombre|precio|stock
 *   DEL_PRODUCTO|id_tienda|id_producto
 *   MOD_PRODUCTO|id_tienda|id_producto|precio|stock
 *   GET_CARTELERA
 *   ADD_PELICULA|id|titulo|sala|horario|filas|columnas
 *   DEL_PELICULA|id
 *   GET_SALA|id_pelicula
 *   RESERVAR|id_pelicula|fila|columna
 *   LOGOUT
 *   EXIT
 */

#define PROTO_PORT      8080
#define PROTO_BUFSIZE   4096
#define PROTO_SEP       "|"
#define PROTO_SEP_CHAR  '|'

/* Códigos de respuesta */
#define PROTO_OK        "OK"
#define PROTO_ERR       "ERR"

/* Comandos */
#define CMD_LOGIN           "LOGIN"
#define CMD_GET_TIENDAS     "GET_TIENDAS"
#define CMD_GET_PRODUCTOS   "GET_PRODUCTOS"
#define CMD_ADD_TIENDA      "ADD_TIENDA"
#define CMD_DEL_TIENDA      "DEL_TIENDA"
#define CMD_ADD_PRODUCTO    "ADD_PRODUCTO"
#define CMD_DEL_PRODUCTO    "DEL_PRODUCTO"
#define CMD_MOD_PRODUCTO    "MOD_PRODUCTO"
#define CMD_GET_CARTELERA   "GET_CARTELERA"
#define CMD_ADD_PELICULA    "ADD_PELICULA"
#define CMD_DEL_PELICULA    "DEL_PELICULA"
#define CMD_GET_SALA        "GET_SALA"
#define CMD_RESERVAR        "RESERVAR"
#define CMD_LOGOUT          "LOGOUT"
#define CMD_EXIT            "EXIT"

#endif /* PROTOCOLO_H */