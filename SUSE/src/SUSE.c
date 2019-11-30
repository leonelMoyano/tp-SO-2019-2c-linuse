/*
 ============================================================================
 Name        : suseServer.c
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 ============================================================================
 */

#include "SUSE.h"

int main(void)
{
	iniciar_logger();
	iniciar_config("/home/utnso/workspace/confBiblioSuse/suseServer.cfg");

	iniciarServidor(g_config_server->puerto, g_logger, (void*)atenderConexion);

	printf("asda2");
	/*
	char* mensaje;
	socketServidor = getSocketServidor(configServer);
	log_info(logger, "Socket servidor creado %d", socketServidor);

	while(1)
	{
		esperarClientes(socketServidor);
		log_info(logger, "Sin conexiones. Esperando clientes Hilolay");
		socketCliente =  getSocketCliente(socketServidor);
		log_info(logger, "Se conecto un cliente Hilolay! %d", socketCliente);
		int cod_op = recibir_operacion(socketCliente);
		switch(cod_op) 
		{
		case MENSAJE:
			mensaje = recibir_mensaje(socketCliente);
			log_info(logger, mensaje);
			free(mensaje);
			continue;
		case -1:
			close(socketCliente);
			log_info(logger, "el cliente se desconecto.%d", socketCliente);
			continue;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	*/
	log_destroy(g_logger);
	return EXIT_SUCCESS;
}


void atenderConexion(int socketCliente) {
	log_debug(g_logger, "Attend connection con este socket %d", socketCliente);
	t_paquete* package = recibirArmarPaquete(socketCliente);
	t_paquete* response;

	log_debug(g_logger, "Checkeo que el paquete sea handshake");
	// Espero recibir el handshake y trato segun quien se conecte
	switch (recibirHandshake(package)) {
	case LIBSUSE:
		;
		log_debug(g_logger, "Recibi el handshake del cliente");

		while (1) {
			package = recibirArmarPaquete(socketCliente);
			log_debug(g_logger, "Recibo paquete");

			if ( package == NULL || package->codigoOperacion == ENVIAR_AVISO_DESCONEXION ) {
				log_warning(g_logger, "Cierro esta conexion del LibMuse %d", socketCliente);
				break;
			};

			response = procesarPaqueteLibMuse(package, socketCliente);
			// enviarPaquetes(socketCliente, response);
			// destruirPaquete(response);
		}
		break;
	default:
		log_warning(g_logger, "El paquete recibido no es handshake");
		break;
	}
	close(socketCliente);
}


void iniciar_config(char* path){
	t_config* config = config_create(path);
	g_config_server = malloc( sizeof( t_config_suse ) );

	g_config_server->puerto = config_get_string_value( config, "LISTEN_PORT" );
	g_config_server->metrics_timer = config_get_int_value( config, "METRICS_TIMER" );
	g_config_server->max_multiprog = config_get_int_value( config, "MAX_MULTIPROG" );

	char ** array = config_get_array_value( config, "SEM_IDS" );
	g_config_server->sem_ids = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_ids, array[ i ] );
	}

	array = config_get_array_value( config, "SEM_INIT" );
	g_config_server->sem_init = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_init, array[ i ] );
	}

	array = config_get_array_value( config, "SEM_MAX" );
	g_config_server->sem_max = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_max, array[ i ] );
	}

	/*
	 * print (char *)list_get(configServer->sem_ids, 0 )
		$3 = 0x804ec20 "A"
		print atoi(list_get(g_config_server->sem_init, 0 ))
		$4 = 0
	 *
	 */

	config_destroy( config );
}

void iniciar_logger(void)
{
	g_logger = log_create("suseServer.log", "SUSE-Server", 1, LOG_LEVEL_INFO);
	log_info(g_logger, "Iniciando SuseServer");
}
