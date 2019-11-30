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

void serializarGet(t_paquete* unPaquete, void* dst,uint32_t src, size_t n){ //size of tipo dst que

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = sizeof(sizeof(void) + sizeof(uint32_t) + sizeof(size_t));
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento, &dst,sizeof(void));
	desplazamiento += sizeof(void);

	memcpy(unPaquete->buffer->data + desplazamiento, &src,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento,&n,sizeof(size_t));
	desplazamiento += sizeof(size_t);
	//CI: Analizar como quedaron los parametros que toma y is es necesario anotrarlos como punteros
}

t_registromget* deserializarGet(t_stream * buffer){
	t_registromget * registro = malloc(sizeof(t_registromget));
	int desplazamiento = 0;

	memcpy(&registro->dst,buffer->data + desplazamiento,sizeof(void));
	desplazamiento += sizeof(void);

	memcpy(&registro->src,buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&registro->n,buffer->data + desplazamiento, sizeof(size_t));
	return registro;
}
void serialzarCopy(t_paquete* unPaquete, void* src,uint32_t dst, int n){
	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = sizeof(sizeof(void *) + sizeof(uint32_t) + sizeof(size_t));
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento, &dst,sizeof(void));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &src,sizeof(uint32_t));
	desplazamiento += sizeof(void *);

	memcpy(unPaquete->buffer->data + desplazamiento,&n,sizeof(int));
	desplazamiento += sizeof(int);
}

t_registromcopy* deserializarCopy(t_stream * buffer){
	t_registromcopy * registro = malloc(sizeof(t_registromcopy));

	int desplazamiento = 0;

	memcpy(&registro->dst,buffer->data + desplazamiento,sizeof(void *));
	desplazamiento += sizeof(uint32_t);

	memcpy(&registro->src,buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(void *);

	memcpy(&registro->n,buffer->data + desplazamiento, sizeof(int));

	return registro;
}

void serializarMap(t_paquete * unPaquete, char * path, size_t length, int flags){

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamDatos= sizeof(char *)+ sizeof(size_t) + sizeof(int);
	unPaquete->buffer->size = tamDatos;

	unPaquete->buffer->data = malloc(tamDatos);

	int desplazamiento = 0;

	memcpy(unPaquete->buffer->data + desplazamiento, &path, sizeof(char *)); // CI: modificar el n bytes
	desplazamiento += sizeof(char *);

	memcpy(unPaquete->buffer->data + desplazamiento, &length, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &flags, sizeof(int));
	desplazamiento = sizeof(int);
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

void serializarMsync(t_paquete * unPaquete,uint32_t addr, size_t len){
	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = sizeof(uint32_t) + sizeof(size_t);
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	int desplazamiento = 0;

	memcpy(unPaquete->buffer->data + desplazamiento,&addr,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &len,sizeof(size_t));
	desplazamiento += sizeof(size_t);
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

void serializarUnmap(t_paquete* paquete, uint32_t dir){
	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = sizeof(uint32_t);

	paquete->buffer->data = malloc(sizeof(uint32_t));

	memcpy(paquete->buffer->data,&dir,sizeof(uint32_t));
}

t_registrounmap* deserealizarUnmap(t_stream * buffer){
	t_registrounmap* registro = malloc(sizeof(t_registrounmap));

	memcpy(registro->dir,buffer->data,sizeof(uint));

	return registro;
}

//-----------------------------------Respuestas--------------------------------------

void enviarRespuestaAlloc(int server_socket, uint32_t * tamanio) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_ALLOC;

	serializarUINT32(unPaquete, tamanio);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarMuseClose(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_CLOSE;

	serializarMensaje(unPaquete, "Cerrate Loro");

	enviarPaquetes(server_socket, unPaquete);
}


void enviarRespuestaGet(int server_socket, uint32_t * operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_GET;

	serializarUINT32(unPaquete,operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}
void enviarRespuestaCopy(int server_socket, uint32_t * operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_COPY;

	serializarUINT32(unPaquete,operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaMap(int server_socket, uint32_t posicion){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_MAP;

	serializarUINT32(unPaquete,posicion);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaMsync(int server_socket, uint32_t operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_SYNC;

	serializarUINT32(unPaquete,operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarRespuestaUnmap(int server_socket,uint32_t operacionSatisfactoria){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_UNMAP;

	serializarUINT32(unPaquete,operacionSatisfactoria);

	enviarPaquetes(server_socket,unPaquete);
}
