#include "util.h"

void SEGFAULT(){
	 kill(pid,SIGSEGV);
}

void VerificoOperacion(int operacion){
	if(operacion == -1){
		SEGFAULT();
	}
}

void serializarUINT32(t_paquete* unPaquete, uint32_t numero) {
	int tamNumero = sizeof(uint32_t);

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamNumero;
	unPaquete->buffer->data = malloc(tamNumero);

	memcpy(unPaquete->buffer->data, &numero, tamNumero);
}

uint32_t deserializarUINT32(t_stream* buffer) {
	return *(uint32_t*) (buffer->data);
}

void serializarGet(t_paquete* unPaquete, void* dst,uint32_t src, size_t n){ //size of tipo dst que

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));

	void* porcionMemoria = malloc(n);
	memcpy(porcionMemoria + desplazamiento, dst, n);

	int tamanioDatos = n + sizeof(uint32_t) + sizeof(size_t);
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento,&n,sizeof(size_t));
	desplazamiento += sizeof(size_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &src,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento, porcionMemoria , n);
	desplazamiento += n;
}

void serialzarCopy(t_paquete* unPaquete, void* src,uint32_t dst, int n){
	int desplazamiento = 0;

	int cantBytes = n;
	//TODO revisar el malloc de tamanioDatosy void* de src

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = cantBytes + sizeof(uint32_t) + sizeof(int);
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento,&n,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(unPaquete->buffer->data + desplazamiento, &dst,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento, src, cantBytes);
	desplazamiento += cantBytes;
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

void serializarUnmap(t_paquete* paquete, uint32_t dir){
	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = sizeof(uint32_t);

	paquete->buffer->data = malloc(sizeof(uint32_t));

	memcpy(paquete->buffer->data,&dir,sizeof(uint32_t));
}


//-----------------------------------Solicitudes--------------------------------------

void enviarMuseInit(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_INIT;

	serializarMensaje(unPaquete, "Abrite Loro");

	enviarPaquetes(server_socket, unPaquete);
}


void enviarMuseClose(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_CLOSE;

	serializarMensaje(unPaquete, "Cerrate Loro");

	enviarPaquetes(server_socket, unPaquete);
}

void enviarAlloc(int server_socket, uint32_t tamanio) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_ALLOC;

	serializarUINT32(unPaquete, tamanio);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarFree(int server_socket, uint32_t direccionLogica) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_FREE;

	serializarUINT32(unPaquete, direccionLogica);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarGet(int server_socket,void* dst,  uint32_t src, size_t n) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_GET;

	serializarGet(unPaquete,dst, src, n);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarMap(int server_socket,char * path, size_t length, int flags) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_MAP;

	serializarMap(unPaquete, path,  length, flags);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarMsync(int server_socket,uint32_t addr, size_t len) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_SYNC;

	serializarMsync(unPaquete, addr,  len);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarCopy(int server_socket,void* src,  uint32_t dst, int n) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_COPY;

	serialzarCopy(unPaquete,src, dst, n);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarUnmap(int server_socket, uint32_t dir){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_UNMAP;

	serializarUINT32(unPaquete, dir);

	enviarPaquetes(server_socket, unPaquete);
}



