/*
 * cliente.cpp — Cliente del Centro Comercial Deusto (Hito 3)
 * Lenguaje: C++
 * Sockets:  Winsock2 (Windows)
 *
 * Compilar:
 *   g++ cliente.cpp -o cliente.exe -lws2_32
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iomanip>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

extern "C" {
#include "protocolo.h"
}

/* ================================================================== */
/*  Estructuras de datos en memoria (caché local — minimiza peticiones)*/
/* ================================================================== */

struct Producto {
    int         id;
    std::string nombre;
    float       precio;
    int         stock;
};

struct Tienda {
    int                   id;
    std::string           nombre;
    int                   numProductos;
    std::vector<Producto> inventario;   /* cargado bajo demanda */
    bool                  inventarioCargado = false;
};

struct Pelicula {
    int         id;
    std::string titulo;
    int         sala;
    std::string horario;
    int         filas;
    int         columnas;
    std::vector<std::vector<int>> asientos; /* cargado bajo demanda */
    bool        salasCargada = false;
};

/* ================================================================== */
/*  Clase Conexion — encapsula el socket                               */
/* ================================================================== */
class Conexion {
public:
    Conexion() : sock(INVALID_SOCKET) {}

    bool conectar(const std::string& host, int puerto) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return false;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) return false;

        SOCKADDR_IN addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(puerto);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(sock); sock = INVALID_SOCKET; return false;
        }
        return true;
    }

    /* Envía comando y devuelve respuesta completa */
    std::string enviar(const std::string& cmd) {
        std::string msg = cmd + "\n";
        send(sock, msg.c_str(), (int)msg.size(), 0);
        return recibir();
    }

    void cerrar() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        WSACleanup();
    }

    bool valido() const { return sock != INVALID_SOCKET; }

private:
    SOCKET sock;

    std::string recibir() {
        std::string result;
        char c;
        while (true) {
            int r = recv(sock, &c, 1, 0);
            if (r <= 0) break;
            if (c == '\n') break;
            if (c == '\r') continue;
            result += c;
        }
        return result;
    }
};

/* ================================================================== */
/*  Clase ClienteCC — lógica de negocio del cliente                   */
/* ================================================================== */
class ClienteCC {
public:
    explicit ClienteCC(Conexion& con) : conn(con) {}

    /* --- Autenticación -------------------------------------------- */
    bool login(const std::string& user, const std::string& pass) {
        std::string resp = conn.enviar(std::string(CMD_LOGIN) + "|" + user + "|" + pass);
        if (resp.substr(0,2) == "OK") {
            usuario = user;
            return true;
        }
        return false;
    }

    void logout() {
        conn.enviar(CMD_LOGOUT);
        usuario.clear();
        limpiarCache();
    }

    /* --- Tiendas -------------------------------------------------- */
    const std::vector<Tienda>& getTiendas(bool forzar = false) {
        if (tiendas.empty() || forzar) {
            tiendas.clear();
            std::string resp = conn.enviar(CMD_GET_TIENDAS);
            if (resp.substr(0,2) != "OK") return tiendas;
            /* Parsear: OK|id;nombre;nProd|id;nombre;nProd|... */
            std::istringstream ss(resp.substr(3));
            std::string token;
            while (std::getline(ss, token, '|')) {
                if (token.empty()) continue;
                Tienda t;
                sscanf(token.c_str(), "%d", &t.id);
                auto p1 = token.find(';');
                auto p2 = token.find(';', p1+1);
                t.nombre       = token.substr(p1+1, p2-p1-1);
                t.numProductos = std::stoi(token.substr(p2+1));
                tiendas.push_back(t);
            }
        }
        return tiendas;
    }

    const std::vector<Producto>& getProductos(int idTienda) {
        for (auto& t : tiendas) {
            if (t.id == idTienda) {
                if (!t.inventarioCargado) {
                    t.inventario.clear();
                    std::string resp = conn.enviar(
                        std::string(CMD_GET_PRODUCTOS) + "|" + std::to_string(idTienda));
                    if (resp.substr(0,2) == "OK") {
                        std::istringstream ss(resp.substr(3));
                        std::string token;
                        while (std::getline(ss, token, '|')) {
                            if (token.empty()) continue;
                            Producto p;
                            char nombre[200];
                            sscanf(token.c_str(), "%d;%199[^;];%f;%d",
                                   &p.id, nombre, &p.precio, &p.stock);
                            p.nombre = nombre;
                            t.inventario.push_back(p);
                        }
                    }
                    t.inventarioCargado = true;
                }
                return t.inventario;
            }
        }
        static std::vector<Producto> vacio;
        return vacio;
    }

    std::string addTienda(int id, const std::string& nombre) {
        std::string resp = conn.enviar(
            std::string(CMD_ADD_TIENDA) + "|" + std::to_string(id) + "|" + nombre);
        if (resp.substr(0,2) == "OK") { tiendas.clear(); }
        return mensaje(resp);
    }

    std::string delTienda(int id) {
        std::string resp = conn.enviar(
            std::string(CMD_DEL_TIENDA) + "|" + std::to_string(id));
        if (resp.substr(0,2) == "OK") { tiendas.clear(); }
        return mensaje(resp);
    }

    std::string addProducto(int id, int idTienda, const std::string& nombre,
                             float precio, int stock) {
        std::ostringstream oss;
        oss << CMD_ADD_PRODUCTO << "|" << id << "|" << idTienda << "|"
            << nombre << "|" << std::fixed << std::setprecision(2)
            << precio << "|" << stock;
        std::string resp = conn.enviar(oss.str());
        if (resp.substr(0,2) == "OK") invalidarTienda(idTienda);
        return mensaje(resp);
    }

    std::string delProducto(int idTienda, int idProducto) {
        std::string resp = conn.enviar(
            std::string(CMD_DEL_PRODUCTO) + "|" + std::to_string(idTienda)
            + "|" + std::to_string(idProducto));
        if (resp.substr(0,2) == "OK") invalidarTienda(idTienda);
        return mensaje(resp);
    }

    std::string modProducto(int idTienda, int idProducto, float precio, int stock) {
        std::ostringstream oss;
        oss << CMD_MOD_PRODUCTO << "|" << idTienda << "|" << idProducto
            << "|" << std::fixed << std::setprecision(2) << precio << "|" << stock;
        std::string resp = conn.enviar(oss.str());
        if (resp.substr(0,2) == "OK") invalidarTienda(idTienda);
        return mensaje(resp);
    }

    /* --- Cartelera ------------------------------------------------ */
    const std::vector<Pelicula>& getCartelera(bool forzar = false) {
        if (cartelera.empty() || forzar) {
            cartelera.clear();
            std::string resp = conn.enviar(CMD_GET_CARTELERA);
            if (resp.substr(0,2) != "OK") return cartelera;
            std::istringstream ss(resp.substr(3));
            std::string token;
            while (std::getline(ss, token, '|')) {
                if (token.empty()) continue;
                Pelicula p;
                char titulo[200], horario[20];
                sscanf(token.c_str(), "%d;%199[^;];%d;%19[^;];%d;%d",
                       &p.id, titulo, &p.sala, horario, &p.filas, &p.columnas);
                p.titulo  = titulo;
                p.horario = horario;
                cartelera.push_back(p);
            }
        }
        return cartelera;
    }

    std::string addPelicula(int id, const std::string& titulo, int sala,
                             const std::string& horario, int filas, int cols) {
        std::ostringstream oss;
        oss << CMD_ADD_PELICULA << "|" << id << "|" << titulo << "|"
            << sala << "|" << horario << "|" << filas << "|" << cols;
        std::string resp = conn.enviar(oss.str());
        if (resp.substr(0,2) == "OK") cartelera.clear();
        return mensaje(resp);
    }

    std::string delPelicula(int id) {
        std::string resp = conn.enviar(
            std::string(CMD_DEL_PELICULA) + "|" + std::to_string(id));
        if (resp.substr(0,2) == "OK") cartelera.clear();
        return mensaje(resp);
    }

    /* Devuelve matriz de asientos (usa caché local) */
    std::vector<std::vector<int>> getSala(int idPelicula) {
        for (auto& p : cartelera) {
            if (p.id == idPelicula) {
                if (!p.salasCargada) {
                    std::string resp = conn.enviar(
                        std::string(CMD_GET_SALA) + "|" + std::to_string(idPelicula));
                    if (resp.substr(0,2) == "OK") {
                        /* OK|filas|cols|0101... */
                        std::istringstream ss(resp.substr(3));
                        std::string sf, sc, datos;
                        std::getline(ss, sf,  '|');
                        std::getline(ss, sc,  '|');
                        std::getline(ss, datos,'|');
                        int filas = std::stoi(sf), cols = std::stoi(sc);
                        p.asientos.assign(filas, std::vector<int>(cols, 0));
                        for (int f = 0; f < filas; f++)
                            for (int c = 0; c < cols; c++)
                                p.asientos[f][c] = datos[f*cols+c] - '0';
                        p.salasCargada = true;
                    }
                }
                return p.asientos;
            }
        }
        return {};
    }

    std::string reservar(int idPelicula, int fila, int col) {
        std::string resp = conn.enviar(
            std::string(CMD_RESERVAR) + "|" + std::to_string(idPelicula)
            + "|" + std::to_string(fila) + "|" + std::to_string(col));
        if (resp.substr(0,2) == "OK") invalidarPelicula(idPelicula);
        return mensaje(resp);
    }

    const std::string& getUsuario() const { return usuario; }

private:
    Conexion&            conn;
    std::string          usuario;
    std::vector<Tienda>  tiendas;
    std::vector<Pelicula>cartelera;

    void limpiarCache() { tiendas.clear(); cartelera.clear(); }

    void invalidarTienda(int id) {
        for (auto& t : tiendas)
            if (t.id == id) { t.inventarioCargado = false; break; }
    }

    void invalidarPelicula(int id) {
        for (auto& p : cartelera)
            if (p.id == id) { p.salasCargada = false; break; }
    }

    static std::string mensaje(const std::string& resp) {
        auto pos = resp.find('|');
        return pos == std::string::npos ? resp : resp.substr(pos+1);
    }
};

/* ================================================================== */
/*  Menú — funciones de presentación                                   */
/* ================================================================== */
static void limpiar() { system("cls"); }
static void pausar()  {
    std::cout << "\nPresione Enter para continuar...";
    std::cin.ignore(10000, '\n');
}

static void mostrarTiendas(ClienteCC& cc) {
    const auto& ts = cc.getTiendas();
    std::cout << "\n========== LISTADO DE TIENDAS ==========\n";
    if (ts.empty()) { std::cout << "No hay tiendas.\n"; return; }
    for (const auto& t : ts)
        std::cout << "ID: " << t.id << " | Nombre: " << t.nombre
                  << " | Productos: " << t.numProductos << "\n";
    std::cout << "========================================\n";
}

static void mostrarCartelera(ClienteCC& cc) {
    const auto& ps = cc.getCartelera();
    std::cout << "\n========== CARTELERA ==========\n";
    if (ps.empty()) { std::cout << "No hay películas.\n"; return; }
    for (const auto& p : ps)
        std::cout << "ID: " << p.id << " | " << p.titulo
                  << " | Sala " << p.sala << " | " << p.horario
                  << " | " << p.filas << "x" << p.columnas << "\n";
    std::cout << "================================\n";
}

/* ---- Submenú Tiendas ---- */
static void menuTiendas(ClienteCC& cc) {
    int op;
    do {
        limpiar();
        mostrarTiendas(cc);
        std::cout << "\n--- GESTION DE TIENDAS ---\n"
                  << "1. Anadir Tienda\n2. Eliminar Tienda\n3. Volver\nSeleccione: ";
        std::cin >> op; std::cin.ignore();

        if (op == 1) {
            int id; std::string nombre;
            std::cout << "ID de la Tienda: "; std::cin >> id; std::cin.ignore();
            std::cout << "Nombre        : "; std::getline(std::cin, nombre);
            std::cout << "\n" << cc.addTienda(id, nombre) << "\n";
            cc.getTiendas(true);
            pausar();
        } else if (op == 2) {
            int id;
            std::cout << "ID de la Tienda a eliminar: "; std::cin >> id; std::cin.ignore();
            std::cout << "\n" << cc.delTienda(id) << "\n";
            cc.getTiendas(true);
            pausar();
        }
    } while (op != 3);
}

/* ---- Submenú Inventario ---- */
static void menuInventario(ClienteCC& cc) {
    if (cc.getTiendas().empty()) {
        std::cout << "\nNo hay tiendas registradas.\n";
        pausar(); return;
    }
    mostrarTiendas(cc);
    int idT;
    std::cout << "ID de la Tienda a gestionar: "; std::cin >> idT; std::cin.ignore();

    const auto& prods = cc.getProductos(idT);

    int op;
    do {
        limpiar();
        std::cout << "\n=== INVENTARIO (Tienda " << idT << ") ===\n";
        std::cout << std::left << std::setw(6) << "ID"
                  << std::setw(25) << "NOMBRE"
                  << std::setw(10) << "PRECIO"
                  << "STOCK\n";
        std::cout << std::string(50, '-') << "\n";
        for (const auto& p : cc.getProductos(idT))
            std::cout << std::setw(6) << p.id
                      << std::setw(25) << p.nombre
                      << std::setw(10) << std::fixed << std::setprecision(2) << p.precio
                      << p.stock << "\n";
        std::cout << "==========================================\n"
                  << "1. Anadir Producto\n2. Eliminar Producto\n"
                  << "3. Modificar Producto\n4. Volver\nSeleccione: ";
        std::cin >> op; std::cin.ignore();

        if (op == 1) {
            int id, stock; float precio; std::string nombre;
            std::cout << "ID del Producto  : "; std::cin >> id; std::cin.ignore();
            std::cout << "Nombre           : "; std::getline(std::cin, nombre);
            std::cout << "Precio           : "; std::cin >> precio; std::cin.ignore();
            std::cout << "Stock            : "; std::cin >> stock; std::cin.ignore();
            std::cout << "\n" << cc.addProducto(id, idT, nombre, precio, stock) << "\n";
            pausar();
        } else if (op == 2) {
            int id;
            std::cout << "ID del Producto a eliminar: "; std::cin >> id; std::cin.ignore();
            std::cout << "\n" << cc.delProducto(idT, id) << "\n";
            pausar();
        } else if (op == 3) {
            int id, stock; float precio;
            std::cout << "ID del Producto a modificar: "; std::cin >> id; std::cin.ignore();
            std::cout << "Nuevo Precio               : "; std::cin >> precio; std::cin.ignore();
            std::cout << "Nuevo Stock                : "; std::cin >> stock; std::cin.ignore();
            std::cout << "\n" << cc.modProducto(idT, id, precio, stock) << "\n";
            pausar();
        }
    } while (op != 4);
}

/* ---- Submenú Cine ---- */
static void menuCine(ClienteCC& cc) {
    int op;
    do {
        limpiar();
        mostrarCartelera(cc);
        std::cout << "\n--- GESTION DE CINE ---\n"
                  << "1. Anadir Pelicula\n2. Eliminar Pelicula\n"
                  << "3. Reservar Asiento\n4. Ver Sala\n5. Volver\nSeleccione: ";
        std::cin >> op; std::cin.ignore();

        if (op == 1) {
            int id, sala, filas, cols; std::string titulo, horario;
            std::cout << "ID de la Pelicula  : "; std::cin >> id; std::cin.ignore();
            std::cout << "Titulo             : "; std::getline(std::cin, titulo);
            std::cout << "Numero de Sala     : "; std::cin >> sala; std::cin.ignore();
            std::cout << "Horario (HH:MM)    : "; std::getline(std::cin, horario);
            std::cout << "Numero de Filas    : "; std::cin >> filas; std::cin.ignore();
            std::cout << "Numero de Columnas : "; std::cin >> cols; std::cin.ignore();
            std::cout << "\n" << cc.addPelicula(id, titulo, sala, horario, filas, cols) << "\n";
            cc.getCartelera(true);
            pausar();
        } else if (op == 2) {
            int id;
            std::cout << "ID de la Pelicula a eliminar: "; std::cin >> id; std::cin.ignore();
            std::cout << "\n" << cc.delPelicula(id) << "\n";
            cc.getCartelera(true);
            pausar();
        } else if (op == 3) {
            int id, fila, col;
            std::cout << "ID de la Pelicula: "; std::cin >> id; std::cin.ignore();
            const auto& sala = cc.getSala(id);
            if (sala.empty()) { std::cout << "Pelicula no encontrada.\n"; pausar(); continue; }
            std::cout << "Fila (1-" << sala.size() << ")   : "; std::cin >> fila; std::cin.ignore();
            std::cout << "Columna (1-" << sala[0].size() << ") : "; std::cin >> col; std::cin.ignore();
            std::cout << "\n" << cc.reservar(id, fila, col) << "\n";
            pausar();
        } else if (op == 4) {
            int id;
            std::cout << "ID de la Pelicula: "; std::cin >> id; std::cin.ignore();
            auto sala = cc.getSala(id);
            if (sala.empty()) { std::cout << "Pelicula no encontrada.\n"; pausar(); continue; }
            /* Buscar título */
            std::string titulo = "?";
            for (const auto& p : cc.getCartelera())
                if (p.id == id) { titulo = p.titulo; break; }
            std::cout << "\n=== SALA " << id << " - " << titulo << " ===\n   ";
            for (int c = 0; c < (int)sala[0].size(); c++)
                std::cout << c+1 << " ";
            std::cout << "\n";
            for (int f = 0; f < (int)sala.size(); f++) {
                std::cout << f+1 << " ";
                for (int c = 0; c < (int)sala[f].size(); c++)
                    std::cout << (sala[f][c] ? "O " : "L ");
                std::cout << "\n";
            }
            pausar();
        }
    } while (op != 5);
}

/* ---- Menú principal ---- */
static void menuPrincipal(ClienteCC& cc) {
    int op;
    do {
        limpiar();
        std::cout << "====================================================\n"
                  << "  === BIENVENIDO AL SISTEMA DEUSTO-CENTRO ===\n"
                  << "  Usuario: " << cc.getUsuario() << "\n"
                  << "====================================================\n"
                  << "1. Gestionar Tiendas\n"
                  << "2. Gestionar Inventario de Tienda\n"
                  << "3. Gestionar Cine\n"
                  << "4. Cerrar Sesion\n"
                  << "====================================================\n"
                  << "Seleccione una opcion: ";
        std::cin >> op; std::cin.ignore();

        if      (op == 1) menuTiendas(cc);
        else if (op == 2) menuInventario(cc);
        else if (op == 3) menuCine(cc);
    } while (op != 4);

    cc.logout();
}

/* ---- Menú de sesión ---- */
static bool menuSesion(ClienteCC& cc) {
    int op;
    do {
        std::cout << "\n====================================================\n"
                  << "     SISTEMA DE GESTION DEUSTO-CENTRO (CLIENTE)\n"
                  << "====================================================\n"
                  << "1. Iniciar sesion\n2. Salir\n"
                  << "====================================================\n"
                  << "Seleccione: ";
        std::cin >> op; std::cin.ignore();

        if (op == 1) {
            std::string user, pass;
            std::cout << "\n--- INICIO DE SESION ---\n";
            std::cout << "Usuario    : "; std::getline(std::cin, user);
            std::cout << "Contrasena : "; std::getline(std::cin, pass);
            if (cc.login(user, pass)) {
                std::cout << "\nBienvenido, " << user << ".\n";
                return true;
            } else {
                std::cout << "\nCredenciales incorrectas.\n";
            }
        }
    } while (op != 2);
    return false;
}

/* ================================================================== */
/*  Main                                                                */
/* ================================================================== */
int main() {
    std::cout << "====================================================\n"
              << "     CLIENTE DEUSTO-CENTRO (Hito 3)\n"
              << "====================================================\n";

    /* Leer cliente_config.txt */
    std::string host = "127.0.0.1";
    int puerto = PROTO_PORT;
    {
        std::ifstream cfg_file("cliente_config.txt");
        if (cfg_file.is_open()) {
            std::string linea;
            while (std::getline(cfg_file, linea)) {
                auto pos = linea.find('=');
                if (pos == std::string::npos) continue;
                std::string clave = linea.substr(0, pos);
                std::string valor = linea.substr(pos + 1);
                if (clave == "IP_SERVIDOR")    host   = valor;
                if (clave == "PUERTO_SERVIDOR") puerto = std::stoi(valor);
            }
            std::cout << "Configuracion cargada desde 'cliente_config.txt'.\n";
        } else {
            std::cout << "cliente_config.txt no encontrado. Usando valores por defecto.\n";
        }
    }

    /* Conectar */
    Conexion conn;
    std::cout << "Conectando a " << host << ":" << puerto << "...\n";
    if (!conn.conectar(host, puerto)) {
        std::cerr << "Error: no se pudo conectar al servidor.\n";
        return 1;
    }
    std::cout << "Conexion establecida.\n";

    ClienteCC cc(conn);

    /* Bucle de sesión: permite múltiples logins sin reconectar */
    while (true) {
        if (!menuSesion(cc)) break;
        menuPrincipal(cc);
        /* Si salió del menú principal por "Cerrar Sesion", vuelve al login */
    }

    conn.enviar(CMD_EXIT);
    conn.cerrar();

    std::cout << "\n====================================================\n"
              << "     CLIENTE DEUSTO-CENTRO FINALIZADO\n"
              << "====================================================\n";
    return 0;
}