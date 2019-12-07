/*
 ============================================================================
 Name        : suseServer.c
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 ============================================================================
 */

#include "SUSE.h"

void atenderConexion(int socketCliente);

int main(void)
{
	iniciar_logger();
	iniciar_config("/home/utnso/workspace/confBiblioSuse/suseServer.cfg");

	iniciarServidor(g_config_server->puerto, g_logger, (void*)atenderConexion);

	printf("asda2");
	/*
	while(1)
	{
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

	config_destroy( g_config );
	return EXIT_SUCCESS;
}


void atenderConexion(int socketCliente) {
	// TODO seguir por aca
	log_debug(g_logger, "Attend connection con este socket %d", socketCliente);
	t_paquete* package = recibirArmarPaquete(socketCliente);
	t_paquete* response;

	log_debug(g_logger, "Checkeo que el paquete sea handshake");
	// Espero recibir el handshake y trato segun quien se conecte
	switch (recibirHandshake(package)) {
	case BIBLIO_SUSE_CLIENT_ID:
		;
		log_debug(g_logger, "Recibi el handshake del cliente");

		enviarMultiProg( socketCliente );
		while (1) {
			package = recibirArmarPaquete(socketCliente);
			log_debug(g_logger, "Recibo paquete");

			if ( package == NULL || package->codigoOperacion == ENVIAR_AVISO_DESCONEXION ) {
				log_warning(g_logger, "Cierro esta conexion del LibMuse %d", socketCliente);
				break;
			};

			// response = procesarPaqueteLibMuse(package, socketCliente);
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

void enviarMultiProg( int socket_dst ){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));
	unPaquete->codigoOperacion = SUSE_GRADO_MULTIPROG;

	log_debug(g_logger, "Recibi el handshake del cliente %d", g_config_server->max_multiprog);
	serializarNumero(unPaquete, g_config_server->max_multiprog);
	enviarPaquetes(socket_dst, unPaquete);
}


void iniciar_config(char* path){
	g_config = config_create(path);
	g_config_server = malloc( sizeof( t_config_suse ) );

	g_config_server->puerto = config_get_string_value( g_config, "LISTEN_PORT" );
	g_config_server->metrics_timer = config_get_int_value( g_config, "METRICS_TIMER" );
	g_config_server->max_multiprog = config_get_int_value( g_config, "MAX_MULTIPROG" );

	char ** array = config_get_array_value( g_config, "SEM_IDS" );
	g_config_server->sem_ids = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_ids, array[ i ] );
	}

	array = config_get_array_value( g_config, "SEM_INIT" );
	g_config_server->sem_init = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_init, array[ i ] );
	}

	array = config_get_array_value( g_config, "SEM_MAX" );
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
}

void iniciar_logger(void)
{
	g_logger = log_create("suseServer.log", "SUSE-Server", 1, LOG_LEVEL_INFO);
	log_info(g_logger, "Iniciando SuseServer");
}
