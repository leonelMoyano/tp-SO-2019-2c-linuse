/*
 * suseDefs.h
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 */

#ifndef SUSEDEFS_H_
#define SUSEDEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <biblioSuse/biblioSuse.h>
#include <hilolay/internal.h>
#include <hilolay/alumnos.h>
#include <hilolay/hilolay.h>

#define IP "127.0.0.1"
#define PUERTO "30001"

typedef enum
{
	MENSAJE,
	PAQUETE
} op_code;

t_log* logger;

void* 	recibir_buffer		(int*, int);
int 	iniciar_servidor	(void);
int 	esperar_cliente		(int);
t_list* recibir_paquete		(int);
void 	recibir_mensaje		(int);
int 	recibir_operacion	(int);

#endif /* SUSEDEFS_H_ */
