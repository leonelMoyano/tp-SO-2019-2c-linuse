
#include <stdio.h>
#include <stdlib.h>
#include <biblioteca/paquetes.h>
#include "MUSE.h"

int main(void) {

	g_logger = log_create("MUSE.log", "MUSE", true, LOG_LEVEL_TRACE);
	g_loggerDebug = log_create("MUSE.log", "MUSE", false, LOG_LEVEL_DEBUG);

	log_info( g_logger, "Inicio proceso de MUSE" );

	//armarConfigMemoria(); va a romper por ruta configurada;

	iniciarServer(g_configuracion->puertoConexion, gestionarSolicitudes(),g_logger);

	return prueba();

}


void armarConfigMemoria() {

	log_info( g_logger, "Leyendo config: %s", RUTACONFIG );

	g_config = config_create(RUTACONFIG);
	g_configuracion = malloc( sizeof( g_configuracion ) );

	g_configuracion->puertoConexion    = config_get_int_value(g_config, "LISTEN_PORT");
	g_configuracion->tamanioMemoria    = config_get_int_value(g_config, "MEMORY_SIZE");
	g_configuracion->tamanioSwap    = config_get_int_value(g_config, "PAGE_SIZE");
	g_configuracion->tamanioPagina    = config_get_int_value(g_config, "SWAP_SIZE");


}



void reservarEspacioMemoriaPrincipal(){

	log_debug( g_loggerDebug, "Reservando memoria (bytes) %d", g_configuracion->tamanioMemoria );
	g_granMalloc = malloc( g_configuracion->tamanioMemoria );

	g_cantidadRegistrosPosibles = ( int )( g_configuracion->tamanioMemoria /  g_configuracion->tamanioPagina );
	char * data = malloc( g_cantidadRegistrosPosibles );
	memset(data, 0, g_cantidadRegistrosPosibles); // Inicializo todos los marcos en 0 ( libres
	g_bitarray_marcos = bitarray_create_with_mode(data, g_cantidadRegistrosPosibles, MSB_FIRST);

	maxPaginasEnSwap =  ( int )( g_configuracion->tamanioSwap / g_configuracion->tamanioPagina );
	char * data = malloc( maxPaginasEnSwap );
	memset(data, 0, maxPaginasEnSwap); // Inicializo todos los marcos en 0 ( libres )
	g_bitarray_swap = bitarray_create_with_mode(data, maxPaginasEnSwap, MSB_FIRST);

}
