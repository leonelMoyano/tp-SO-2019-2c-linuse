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
#include <commons/collections/queue.h>
#include <string.h>

#include "SUSE.h"

#define BIBLIO_SUSE_CLIENT_ID 99

typedef struct suse_config{
	char *puerto;
	int metrics_timer;
	int max_multiprog;
	t_list* sem_ids;
	t_list* sem_init;
	t_list* sem_max;
	double alpha_sjf;
} t_config_suse;

typedef enum  {
	SUSE_CREATE = 650,
	SUSE_GRADO_MULTIPROG,
	SUSE_CLOSE,
	SUSE_JOIN,
	SUSE_SCHEDULE_NEXT,
	SUSE_SIGNAL,
	SUSE_WAIT
} t_cod_operaciones_SUSE;

typedef struct suse_semaforo{
	char* nombre;
	int current_value;
	int max_value;
} t_semaforo_suse;

typedef struct suse_cliente_thread{
	int tid;
	time_t time_created;
	time_t time_last_run;
	time_t time_last_yield;
} t_client_thread_suse;

typedef struct suse_cliente{
	int main_tid;
	t_client_thread_suse* running_thread;
	t_queue* new;
	t_list* blocked;
	t_list* waiting;
	t_list* exit;
} t_client_suse;

t_config_suse* g_config_server;

void        iniciar_config      (char* path);
void 		esperarClientes		(int);
void        enviarMultiProg     ( int socket_dst );
// int 		getSocketCliente	(int);
void* 		recibir_buffer		(int*,int);
int 		recibir_operacion	(int);
char* 		recibir_mensaje		(int);

/*********** algunas utils ***********/
int tamanio_array(char** array);

#endif /* SUSEDEFS_H_ */
