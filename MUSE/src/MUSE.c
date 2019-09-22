
#include <stdio.h>
#include <stdlib.h>
#include <biblioteca/paquetes.h>
#include "MUSE.h"

int main(void) {
	return prueba();
}

void destruirGlobales(){
	config_destroy(g_configuracion);
	log_destroy(g_logger);
	log_destroy(g_loggerDebug);
}


void reservarEspacioMemoriaPrincipal(){

	log_debug( g_loggerDebug, "Reservando memoria (bytes) %d", g_configuracion->tamanioMemoria );
	g_granMalloc = malloc( g_configuracion->tamanioMemoria );

	//g_cantidadRegistrosPosibles = ( int )( g_configuracion->tamanioMemoria / g_tamanioRegistro );

	//char * data = malloc( g_cantidadRegistrosPosibles );
	//memset(data, 0, g_cantidadRegistrosPosibles); // Inicializo todos los marcos en 0 ( libres )

	//g_bitarray_marcos = bitarray_create_with_mode(data, g_cantidadRegistrosPosibles, MSB_FIRST);
}
