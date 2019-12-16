/*
 * sac_server_serializaciones.h
 *
 *  Created on: 15 dic. 2019
 *      Author: utnso
 */

#ifndef SAC_SERVER_SERIALIZACIONES_H_
#define SAC_SERVER_SERIALIZACIONES_H_

#include <biblioNOC/paquetes.h>

typedef struct fuse_write_response{
	char* path;
	time_t modified_time;
} t_utimens_request;

// Serializaciones
t_paquete* armarPaqueteReturnErrnoConOperacion( int return_value, int errno_code, int codigo_op );

// Deserializaciones
t_utimens_request* deserializarUtimensReq( t_stream* buffer );

// Destruir structs
void destruirUtimensReq( t_utimens_request* utimens_req );

#endif /* SAC_SERVER_SERIALIZACIONES_H_ */
