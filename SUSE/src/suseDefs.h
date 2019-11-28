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
#include <string.h>
#include <biblioSuse/biblioSuse.h>
#include <biblioSuse/libSuseUtils.h>
#include <hilolay/internal.h>
#include <hilolay/alumnos.h>
#include <hilolay/hilolay.h>


int 		getSocketServidor	(t_config*);
void 		esperarClientes		(int);
int 		getSocketCliente	(int);
void* 		recibir_buffer		(int*,int);
int 		recibir_operacion	(int);
char* 		recibir_mensaje		(int);

#endif /* SUSEDEFS_H_ */
