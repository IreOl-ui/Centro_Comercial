#include <stdio.h>
#include <stdlib.h>
#include "centro_comercial.h"
#include "menu.h"
#include "persistencia.h"

#define ARCHIVO_DATOS "deusto_centro_data.txt"

int main() {
    printf("\n====================================================\n");
    printf("       INICIANDO SISTEMA DEUSTO-CENTRO\n");
    printf("====================================================\n");
    
    // Crear centro comercial
    CentroComercial* cc = cc_crear();
    if (cc == NULL) {
        printf("Error crítico: No se pudo inicializar el sistema.\n");
        return 1;
    }
    
    // Cargar datos existentes
    if (cargar_datos(cc, ARCHIVO_DATOS)) {
        printf("Datos cargados correctamente desde '%s'.\n", ARCHIVO_DATOS);
    } else {
        printf("No se encontró archivo de datos previo. Se creará uno nuevo al guardar.\n");
    }
    
    printf("\nPresione Enter para continuar...");
    getchar();
    
    // Iniciar menú principal
    menu_principal(cc);
    
    // Liberar memoria
    cc_liberar(cc);
    
    printf("\n====================================================\n");
    printf("       SISTEMA DEUSTO-CENTRO FINALIZADO\n");
    printf("====================================================\n");
    
    return 0;
}