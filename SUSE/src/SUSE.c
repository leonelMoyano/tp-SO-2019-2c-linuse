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
	void iterator(char* value)
	{
		printf("%s\n", value);
	}
	int socketServidor, socketCliente;
	iniciar_logger();
	configServer = leer_config("/home/utnso/workspace/confBiblioSuse/suseServer.cfg");
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
	close(socketServidor);
	config_destroy(configServer);
	log_destroy(logger);
	return EXIT_SUCCESS;
}

void iniciar_logger(void)
{
	logger = log_create("suseServer.log", "SUSE-Server", 1, LOG_LEVEL_INFO);
	log_info(logger, "Iniciando SuseServer");
}
