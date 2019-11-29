#include "libMUSE.h"
#include <stdio.h>
#include <stdlib.h>
#include <biblioteca/paquetes.h>

int main(void) {
	return prueba();
}

int muse_init(int id, char* ip, int puerto){

	socketConexion = conectarCliente(ip,puerto,id);

	enviarMensaje(socketConexion, "");
	//necesito proceso id, e hilo id?

	return socket;

}

void muse_close(){

	//limpiar estructuras, liberar memoria
}

uint32_t muse_alloc(uint32_t tam){

	enviarAlloc(socketConexion, tam);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t * direccionLogica = deserializarUINT32(paquete->buffer);

	return direccionLogica;

}

void muse_free(uint32_t dir){

	enviarFree(socketConexion, dir);//CI: Deberia pasatle a MUSE la pocision del frame a liberar o Muse debe encontrarlo
}

int muse_get(void* dst, uint32_t src, size_t n){

	enviarGet(socketConexion,dst,src,n);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t operacionSatisfactoria = deserializarUINT32(paquete->buffer);

	return operacionSatisfactoria;
}

int muse_cpy(uint32_t dst, void* src, int n){

	void * misBytes= malloc(n);

	memcpy(misBytes,src,n);

	enviarCopy(socketConexion, misBytes);

	free(misBytes);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t operacionSatisfactoria = deserializarUINT32(paquete->buffer);

	return operacionSatisfactoria;
}

uint32_t muse_map(char *path, size_t length, int flags){

	enviarMap(socketConexion, path, length, flags);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t pocision = deserializarUINT32(paquete->buffer);

	return pocision;
}

int muse_sync(uint32_t addr, size_t len){

	enviarMsync(socketConexion, addr, len);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t operacionSatisfactoria = deserializarUINT32(paquete->buffer);

	return operacionSatisfactoria;
}

int muse_unmap(uint32_t dir){

	enviarUnmap(socketConexion, dir);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	uint32_t operacionSatisfactoria = deserializarUINT32(paquete->buffer);

	return operacionSatisfactoria;

}


int pruebaReferenciaMUSE(void) {
	puts("!!!prueba biblite si lo camibas de nuevo !!!"); /* prints !!!Hello World!!! */
	return 1;
}
