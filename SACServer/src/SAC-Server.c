/*
 ============================================================================
 Name        : SAC-Server.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

// Este va a ser el servidor del FS centralizado

#include <stdio.h>
#include <stdlib.h>
#include <biblioNOC/conexiones.h>
#include <commons/config.h>

#include "sac_cod_ops.h"
#include "sac_defs.h"
#include "sac_fs_handlers.h"

void       atenderConexion(int socketCliente);
t_paquete* procesarPaqueteFuseOps( t_paquete* request );
void       iniciarConfig( char* path );

int main(void) {
	g_logger = log_create( "/home/utnso/logs/FUSE/server.log", "SACSRVR", 1, LOG_LEVEL_TRACE );
	iniciarConfig( "/home/utnso/workspace/tp-2019-2c-No-C-Nada/configs/FUSE/sacServer.cfg" );
	init_fs( g_fuse_config->disc_file_path );
	iniciarServidor( g_fuse_config->puerto, g_logger, (void*)atenderConexion);
	return EXIT_SUCCESS;
}

void atenderConexion(int socketCliente) {
	log_trace(g_logger, "Attend connection con este socket %d", socketCliente);
	t_paquete* package = recibirArmarPaquete(socketCliente);
	t_paquete* response;

	log_trace(g_logger, "Checkeo que el paquete sea handshake");
	// Espero recibir el handshake y trato segun quien se conecte
	switch (recibirHandshake(package)) {
	case SAC_CLI:
		log_debug(g_logger, "Recibi el handshake del cliente SAC");
		while (1) {
			package = recibirArmarPaquete(socketCliente);
			log_trace(g_logger, "Recibo paquete");

			if ( package == NULL || package->codigoOperacion == ENVIAR_AVISO_DESCONEXION ) {
				log_trace(g_logger, "Cierro esta conexion del sac-cli %d", socketCliente);
				break;
			};

			response = procesarPaqueteFuseOps(package);
			if( response != NULL )
				enviarPaquetes(socketCliente, response);
		}
		break;
	default:
		log_warning(g_logger, "El paquete recibido no es handshake");
		break;
	}

	close(socketCliente);
}

t_paquete* procesarPaqueteFuseOps( t_paquete* request ){
	t_paquete* response = NULL;

	switch( request->codigoOperacion ){
	case SAC_mknod:
		response = procesar_mknod( request );
		break;
	case SAC_utimens:
		response = procesar_utimens( request );
		break;
	case SAC_getattr:
		response = procesar_getattr( request );
		break;
	}

	destruirPaquete( request );
	return response;
}

void iniciarConfig( char* path ){
	t_config* commons_config = config_create( path );
	g_fuse_config = malloc( sizeof( t_fuse_config ) );

	g_fuse_config->puerto = strdup( config_get_string_value( commons_config, "PUERTO" ) );
	g_fuse_config->disc_file_path = strdup( config_get_string_value( commons_config, "DISCO" ) );

	config_destroy( commons_config );
}
