#include "util.h"

void serializarUINT32(t_paquete* unPaquete, uint32_t numero) {
	int tamNumero = sizeof(uint32_t);

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamNumero;
	unPaquete->buffer->data = malloc(tamNumero);

	memcpy(unPaquete->buffer->data, &numero, tamNumero);
}

int deserializarUINT32(t_stream* buffer) {
	return *(uint32_t*) (buffer->data);
}

//TODO verificar serializacion de void*

t_registromget* deserializarGet(t_stream * buffer){
	t_registromget * registro = malloc(sizeof(t_registromget));
	int desplazamiento = 0;

	memcpy(&registro->n,buffer->data + desplazamiento, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	memcpy(&registro->src,buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	registro->dst = malloc(registro->n);
	memcpy(&registro->dst,buffer->data + desplazamiento,sizeof(void));
	desplazamiento += registro->n;

	return registro;
}
t_registromcopy* deserializarCopy(t_stream * buffer){
	t_registromcopy * registro = malloc(sizeof(t_registromcopy));

	int desplazamiento = 0;

	memcpy(&registro->n,buffer->data + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&registro->dst,buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	void * porcionMemoria = malloc(registro->n);
	memcpy(porcionMemoria ,buffer->data + desplazamiento, registro->n);
	desplazamiento += registro->n;


	return registro;
}

t_registromap* deserealizarMap(t_stream * buffer){
	t_registromap* registro = malloc(sizeof(t_registromap));

	int desplazamiento = 0;

	memcpy(&registro->path,&buffer->data + desplazamiento,sizeof(char *));
	desplazamiento += sizeof(char*);

	memcpy(&registro->length,&buffer->data + desplazamiento, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	memcpy(&registro->flags,&buffer->data + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	return registro;
}

t_registrosync* deserealizarMsync(t_stream * buffer){
	t_registrosync* registro = malloc(sizeof(t_registrosync));
	int desplazamiento = 0;

	memcpy(registro->addr, buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(registro->len, buffer->data + desplazamiento, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	return registro;
}

t_registrounmap* deserealizarUnmap(t_stream * buffer){
	t_registrounmap* registro = malloc(sizeof(t_registrounmap));

	memcpy(registro->dir,buffer->data,sizeof(uint));

	return registro;
}

//-----------------------------------Respuestas--------------------------------------

void enviarRespuestaAlloc(int server_socket, uint32_t tamanio) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_ALLOC;

	serializarUINT32(unPaquete, tamanio);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarRespuestaGet(int server_socket, int operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_GET;

	serializarNumero(unPaquete, operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}
void enviarRespuestaCopy(int server_socket, int operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_COPY;

	serializarNumero(unPaquete, operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaMap(int server_socket, uint32_t posicionMemoriaMapeada){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_MAP;

	serializarUINT32(unPaquete, posicionMemoriaMapeada);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaMsync(int server_socket, int operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_SYNC;

	serializarNumero(unPaquete, operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaUnmap(int server_socket,int operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_UNMAP;

	serializarUINT32(unPaquete, operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}
