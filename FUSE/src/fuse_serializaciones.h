/*
 * fuse_serializaciones.h
 *
 *  Created on: 15 dic. 2019
 *      Author: utnso
 */

#ifndef FUSE_SERIALIZACIONES_H_
#define FUSE_SERIALIZACIONES_H_

#include "string.h"
#include <biblioNOC/paquetes.h>

typedef struct fuse_write_response{
        void* buffer;
        size_t size;
        off_t offset;
        t_list* sem_ids;
        t_list* sem_init;
        t_list* sem_max;
        double alpha_sjf;
} t_write_response;

typedef struct fuse_getattr_response{
	int type; // 0 no existe, 1 es archivo, 2 es directorio
	time_t last_mod_time;
	size_t size;
} t_getattr_response;

typedef struct fuse_return_errno_response{
	int return_value;
	int errno_value;
} t_return_errno_response;

t_paquete* armarPaquetePathConOperacion( char* path, int codigo_op );
t_paquete* armarPaqueteUtimens( char* path, time_t new_time, int codigo_op );

t_return_errno_response * deserializarReturnErrno(t_stream * buffer);
t_getattr_response * deserializarGetattr(t_stream * buffer);

#endif /* FUSE_SERIALIZACIONES_H_ */
