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
#include <commons/config.h>
#include <commons/collections/list.h>
#include <string.h>

#include "SUSE.h"

typedef struct suse_config{
	char *puerto;
	int metrics_timer;
	int max_multiprog;
	t_list* sem_ids;
	t_list* sem_init;
	t_list* sem_max;
	double alpha_sjf;
} t_config_suse;

t_config_suse* g_config_server;

void        iniciar_config(char* path);
void 		esperarClientes		(int);
// int 		getSocketCliente	(int);
void* 		recibir_buffer		(int*,int);
int 		recibir_operacion	(int);
char* 		recibir_mensaje		(int);

/*********** algunas utils ***********/
int tamanio_array(char** array);

#endif /* SUSEDEFS_H_ */
