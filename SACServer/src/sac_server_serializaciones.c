/*
 * sac_server_serializaciones.c
 *
 *  Created on: 15 dic. 2019
 *      Author: utnso
 */

#include "sac_server_serializaciones.h"

void serializarDosNumeros(t_paquete* unPaquete, int numero_a, int numero_b){
	int tamTotal = sizeof(int) * 2;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamTotal;
	unPaquete->buffer->data = malloc(tamTotal);

	memcpy(unPaquete->buffer->data, &numero_a, sizeof( int ));
	memcpy(unPaquete->buffer->data + sizeof( int ), &numero_b, sizeof( int ));
}

void serializarGetattr( t_paquete* paquete, int type, time_t last_mod_time, size_t size ){
	int tamTotal = sizeof( int ) + sizeof( time_t ) + sizeof( size_t );
	int desplazamiento = 0;

	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = tamTotal;
	paquete->buffer->data = malloc(tamTotal);

	memcpy(paquete->buffer->data, &type, sizeof( int ));
	desplazamiento += sizeof( int );

	memcpy(paquete->buffer->data + desplazamiento, &last_mod_time, sizeof( time_t ));
	desplazamiento += sizeof( time_t );

	memcpy(paquete->buffer->data + desplazamiento, &size, sizeof( size_t ));
}

t_paquete* armarPaqueteReturnErrnoConOperacion( int return_value, int errno_code, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarDosNumeros( paquete, return_value, errno_code );
	return paquete;
}

t_paquete* armarPaqueteGetattr( int type, time_t last_mod_time, size_t size, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarGetattr( paquete, type, last_mod_time, size );
	return paquete;
}

t_utimens_request* deserializarUtimensReq( t_stream* buffer ){
	t_utimens_request* utimens_req = malloc( sizeof( t_utimens_request ) );

	// path - time_t mod tim
	utimens_req->path = strdup( (char*) buffer->data );
	utimens_req->modified_time = *(time_t*) ( buffer->data + strlen( utimens_req->path ) + 1 );

	return utimens_req;
}

void destruirUtimensReq( t_utimens_request* utimens_req ){
	free( utimens_req->path );
	free( utimens_req );
}
