#include "libMUSE.h"
#include <stdio.h>
#include <stdlib.h>
#include <biblioteca/paquetes.h>

int main(void) {
	return prueba();
}

int muse_init(int id, char* ip, int puerto){


	socketConexion = conectarCliente(ip,puerto,id);

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

	enviarFree(socketConexion, dir);
}

int muse_get(void* dst, uint32_t src, size_t n){

	enviarMensaje(socketConexion, MUSE_GET);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	char * mensaje = deserializarMensaje(paquete->buffer);

	return 1;
}

int muse_cpy(uint32_t dst, void* src, int n){

	enviarMensaje(socketConexion, MUSE_COPY);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	char * mensaje = deserializarMensaje(paquete->buffer);


	int aux = memcpy(src,dst,n);
	return aux;
}

uint32_t muse_map(char *path, size_t length, int flags){

	enviarMensaje(socketConexion, MUSE_MAP);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	char * mensaje = deserializarMensaje(paquete->buffer);

	uint32_t aux = mmap(path,length,0,flags,0,0);
	return aux;
}

int muse_sync(uint32_t addr, size_t len){

	enviarMensaje(socketConexion, MUSE_SYNC);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	char * mensaje = deserializarMensaje(paquete->buffer);

	int aux = msync(addr,len,0);
	return aux;
}

int muse_unmap(uint32_t dir){

	enviarMensaje(socketConexion, MUSE_UNMAP);

	t_paquete * paquete  = recibirArmarPaquete(socketConexion);

	char * mensaje = deserializarMensaje(paquete->buffer);

	int aux = munmap(dir,0);
	return aux;

}


int pruebaReferenciaMUSE(void) {
	puts("!!!prueba biblite si lo camibas de nuevo !!!"); /* prints !!!Hello World!!! */
	return 1;
}
