/*
 * fuse_serializaciones.c
 *
 *  Created on: 15 dic. 2019
 *      Author: utnso
 */
#include "fuse_serializaciones.h"

void serializarWrite( t_paquete* paquete, char* path, void* buffer, size_t size, off_t offset ){
	int largo_path = strlen( path ) + 1;
	int tamTotal = largo_path + sizeof( size_t ) + sizeof( off_t ) + size;

	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = tamTotal;
	paquete->buffer->data = malloc(tamTotal);

	int data_offset = 0;

	strcpy( paquete->buffer->data, path );
	data_offset += largo_path;

	memcpy( paquete->buffer->data + data_offset, &size, sizeof( size_t ) );
	data_offset += sizeof( size_t );

	memcpy( paquete->buffer->data + data_offset, &offset, sizeof( off_t ) );
	data_offset += sizeof( off_t );

	memcpy( paquete->buffer->data + data_offset, buffer, size );
}

void serializarUtimens( t_paquete* paquete, char* path, time_t mod_time ){
	int largo_path = strlen( path ) + 1;
	int tamTotal = largo_path + sizeof( time_t );

	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = tamTotal;
	paquete->buffer->data = malloc(tamTotal);

	int data_offset = 0;

	strcpy( paquete->buffer->data, path );
	data_offset += largo_path;

	memcpy( paquete->buffer->data + data_offset, &mod_time, sizeof( time_t ) );
}

t_return_errno_response * deserializarReturnErrno(t_stream * buffer){
	t_return_errno_response* response = malloc( sizeof( t_return_errno_response ) );

	response->return_value = *(int*) (buffer->data);
	response->errno_value = *(int*) (buffer->data + sizeof( int ));

	return response;
}

t_getattr_response * deserializarGetattr(t_stream * buffer){
	t_getattr_response* response = malloc( sizeof( t_getattr_response ) );
	int desplazamiento = 0;

	response->type = *(int*) (buffer->data);
	desplazamiento += sizeof( int );

	response->last_mod_time = *(time_t*) (buffer->data + desplazamiento);
	desplazamiento += sizeof( time_t );

	response->size = *(size_t*) (buffer->data + desplazamiento);

	return response;
}

t_paquete* armarPaquetePathConOperacion( char* path, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarMensaje( paquete, path );
	return paquete;
}

t_paquete* armarPaqueteUtimens( char* path, time_t new_mod_time, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarUtimens( paquete, path, new_mod_time );
	return paquete;
}

t_paquete* armarPaqueteWrite( char* path, void* buffer, size_t size, off_t offset, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarWrite( paquete, path, buffer, size, offset );
	return paquete;
}
