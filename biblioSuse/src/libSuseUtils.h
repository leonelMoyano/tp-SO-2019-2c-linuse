/*
 * libSuseUtils.h
 *
 *  Created on: 2 nov. 2019
 *      Author: utnso
 */

#ifndef LIBSUSEUTILS_H_
#define LIBSUSEUTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include <commons/config.h>
#include <biblioNOC/conexiones.h>
#include <biblioNOC/paquetes.h>

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct{
	char *ip;
	char *puerto;
} t_config_lib_suse;

int 		crear_conexion		(char* ip, char* puerto);
void 		enviar_mensaje		(char* mensaje, int socket_cliente);
void 		eliminar_paquete	(t_paquete* paquete);
void 		liberar_conexion	(int socket_cliente);
void* 		serializar_paquete	(t_paquete* paquete, int bytes);
t_config* 	leer_config			(char*);
char* 		getAddress			(t_config*);
char* 		getPort				(t_config*);

#endif /* LIBSUSEUTILS_H_ */
