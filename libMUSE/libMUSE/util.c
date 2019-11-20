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
void serializarGet(t_paquete* unPaquete, void * dst,uint32_t src, size_t n){ //size of tipo dst que

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = sizeof(sizeof(void *) + sizeof(uint32_t) + sizeof(size_t));
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento, &dst,sizeof(void));
	desplazamiento += sizeof(void *);

	memcpy(unPaquete->buffer->data + desplazamiento, &src,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento,&n,sizeof(size_t));
	desplazamiento += sizeof(size_t);

}

t_registromget deserializarGEt(t_stream * buffer){
	t_registromget * registro = malloc(sizeof(t_registromget));
	int desplazamiento = 0;

	memcpy(&registro->dst,buffer->data + desplazamiento,sizeof(void));
	desplazamiento += sizeof(void);

	memcpy(&registro->src,buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&registro->n,buffer->data + desplazamiento, sizeof(size_t));
	return registro;
}

void serializarCopy(t_paquete* unPaquete, uint32_t dst, void * bytes){
	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	int tamanioDatos = sizeof(sizeof(void *) + sizeof(void *));
	unPaquete->buffer->size = tamanioDatos;

	unPaquete->buffer->data = malloc(tamanioDatos);

	memcpy(unPaquete->buffer->data + desplazamiento, &dst,sizeof(void));
	desplazamiento += sizeof(uint32_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &bytes,sizeof(void *));
	desplazamiento += sizeof(void *);
}

t_registromcopy deserializarCopy(t_stream buffer){
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

	memcpy(unPaquete->buffer->data + desplazamiento, &path, strlen(path) + 1); // CI: modificar el n bytes
	desplazamiento += strlen(path) + 1;

	memcpy(unPaquete->buffer->data + desplazamiento, &length, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	memcpy(unPaquete->buffer->data + desplazamiento, &flags, sizeof(int));
	desplazamiento = sizeof(int);
}

t_registromap deserealizarMap(t_stream buffer){
	t_registromap registro = malloc(sizeof(t_registromap));

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

t_registrosync deserealizarMsync(t_stream buffer){
	t_registrosync registro = malloc(sizeof(t_registrosync));
	int desplazamiento = 0;

	memcpy(registro->addr, buffer->data + desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(registro->len, buffer->data + desplazamiento, sizeof(size_t));
	desplazamiento += sizeof(size_t);

	return registro;
}

void serializarUnmap(t_paquete paquete, uint32_t dir){
	paquete->buffer = malloc(sizeof(t_stream));
	paquete->buffer->size = sizeof(uint32_t);

	paquete->buffer->data = malloc(sizeof(uint32_t));

	memcpy(paquete->buffer->data,&dir,sizeof(uint32_t));
}

t_registrounmap deserealizarUnmap(t_stream buffer){
	t_registrounmap registro = malloc(sizeof(t_registrounmap));

	memcpy(registro->dir,buffer->data,sizeof(uint));

	return registro;
}


//-------------------------------Envio de paquetes----------------------------------------------

void enviarMuseInit(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_INIT;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}


void enviarAlloc(int server_socket, uint32_t * tamanio) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_ALLOC;

	serializarUINT32(unPaquete, tamanio);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarFree(int server_socket, uint32_t * dir) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_FREE;

	serializarUINT32(unPaquete, dir);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarGet(int server_socket,void *  dst, uint32_t src, size_t n){  //CI: dst
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_GET;

	serializarGet(unPaquete,dst,src,n);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarCopy(int server_socket,uint32_t dst, void * bytes){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_COPY;

	serializarCopy(unPaquete,dst,bytes);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarMap(int server_socket,char *path, size_t length, int flags){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_MAP;

	serializarMap(unPaquete, path,length,flags); //CI: DOnde va a hacer la valdacion con el tamaño del archivo

	enviarPaquetes(server_socket,unPaquete);
}

void enviarMsync(int server_socket, uint32_t addr, size_t len){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_SYNC;

	serializarMsync(unPaquete,addr,len);

	enviarPaquetes(server_socket,unPaquete);
}

void enviarUnmap(int server_socket,uint32_t dir){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = MUSE_UNMAP;

	serializarMsync(unPaquete,dir);

	enviarPaquetes(server_socket,unPaquete);
}

