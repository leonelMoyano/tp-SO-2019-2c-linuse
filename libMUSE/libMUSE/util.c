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

