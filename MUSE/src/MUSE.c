#include "MUSE.h"

int main(void) {

	idSegmento = 0;
	punteroClock = 0;
	nroPrograma = 1;
	direccionamientoLogicoActual = 0;
	tamanio_heap = 5;

	g_logger = log_create("MUSE.log", "MUSE", true, LOG_LEVEL_TRACE);
	g_loggerDebug = log_create("MUSEDebug.log", "MUSE", false, LOG_LEVEL_DEBUG);
	log_info( g_logger, "Inicio proceso de MUSE" );

	programas = crearTablaProgramas();
	paginasEnSwap = crearListaPaginasSwap();
	mapeosAbiertosCompartidos = crearListaMapeos();
	tablasDePaginas = crearTablaPaginas();

	sem_init(&g_mutexTablaProgramas, 0, 1);
	sem_init(&g_mutextablasDePaginas, 0, 1);
	sem_init(&g_mutexMapeosAbiertosCompartidos, 0, 1);
	sem_init(&g_mutexPaginasEnSwap, 0, 1);
	sem_init(&g_mutexSwap, 0, 1);
	sem_init(&g_mutexgBitarray_swap, 0, 1);
	sem_init(&g_mutexgBitarray_marcos, 0, 1);

	armarConfigMemoria();

	lengthPagina = g_configuracion->tamanioPagina;
	size_t tamArch;

	//archivoSwap = malloc(g_configuracion->tamanioSwap);
	archivoSwap = abrirArchivo(RUTASWAP,&tamArch,&disco_swap);

	iniciarServidor(g_configuracion->puertoConexion,g_logger, (void*)attendConnection);

	return prueba();

}

int arrancarServer(char* puertostring){

	int ok = iniciarServidor(puertostring, g_logger, (void*)attendConnection);
	return ok;
}

void attendConnection( int socketCliente) {
	// int socketCliente = *(int *)socket_fd;
	log_debug( g_loggerDebug, "Attend connection con este socket %d", socketCliente );
	t_paquete* package = recibirArmarPaquete(socketCliente);
	t_paquete* response;

	log_debug( g_loggerDebug, "Checkeo que el paquete sea handshake" );
	// Espero recibir el handshake y trato segun quien se conecte
	switch(recibirHandshake(package)){
		case LIBMUSE: ;
			log_debug( g_loggerDebug, "Recibi el handshake del cliente" );

			while (1) {
				package = recibirArmarPaquete(socketCliente);
				log_debug( g_loggerDebug, "Recibo paquete" );

				if ( package == NULL || package->codigoOperacion == ENVIAR_AVISO_DESCONEXION ){
					log_warning( g_loggerDebug, "Cierro esta conexion del LibMuse %d", socketCliente );
					break;
				};

				response = procesarPaqueteLibMuse( package, socketCliente );
				// enviarPaquetes(socketCliente, response);
				// destruirPaquete(response);
			}
			break;
		default:
			log_warning( g_loggerDebug, "El paquete recibido no es handshake" );
			break;
	}
	close(socketCliente);
	// removeThreadFromActualThreads( pthread_self() );
}


t_paquete* procesarPaqueteLibMuse(t_paquete* paquete, int cliente_fd) {

	int socket = cliente_fd;

	log_debug( g_loggerDebug, "Proceso codigo op %d", paquete->codigoOperacion );

	switch (paquete->codigoOperacion) {

	/* nunca entra por aca porque el handshake lo recibo cuando entro a "attendConnection"
	case HANDSHAKE:
		procesarHandshake(paquete, cliente_fd);
		break;
    */
	case MUSE_INIT: ;
	    InicializarNuevoPrograma(socket);
		break;

	case MUSE_CLOSE: ;
		FinalizarPrograma(socket);
		break;

	case MUSE_ALLOC: ;
		uint32_t tamanio  = deserializarUINT32(paquete->buffer);
		uint32_t direccionLogica = procesarAlloc(tamanio, socket);
		enviarRespuestaAlloc(cliente_fd,direccionLogica);
		break;

	case MUSE_FREE: ;
		uint32_t direccionLogicaFree = deserializarUINT32(paquete->buffer);
		procesarFree(direccionLogicaFree,socket); //Libera una porcion de memoria reservada
		break;

	case MUSE_GET: ;
		t_registromget* registroGet = deserializarGet(paquete->buffer);

		uint32_t operacionSatisfactoriaGet = procesarGet(registroGet->dst,registroGet->src,registroGet->n,socket);

		enviarRespuestaGet(cliente_fd, operacionSatisfactoriaGet);
		break;

	case MUSE_COPY: ;
		t_registromcopy* registroCopy = deserializarCopy(paquete->buffer);

		uint32_t operacionSatisfactoriaCopy = procesarCopy( registroCopy->dst,registroCopy->src,registroCopy->n,socket);

		enviarRespuestaCopy(cliente_fd, operacionSatisfactoriaCopy);
		break;

	case MUSE_MAP: ;
		t_registromap* registroMap = deserealizarMap(paquete->buffer);

		uint32_t pocision = procesarMap(registroMap->path,registroMap->length,registroMap->flags,socket);

		enviarRespuestaMap(cliente_fd, pocision);

		break;

	case MUSE_SYNC: ;
		t_registrosync* registroSync = deserealizarMsync(paquete->buffer);

		uint32_t resultadoSync =  procesarSync( registroSync->addr,registroSync->len,socket);

		enviarRespuestaMsync(cliente_fd,resultadoSync);

		break;

	case MUSE_UNMAP: ;
		t_registrounmap* registroUnmap = deserealizarUnmap(paquete->buffer);

		uint32_t resultadoUnMap =  procesarUnMap(registroUnmap->dir,socket);

		enviarRespuestaUnmap(cliente_fd,resultadoUnMap);

		break;

	default:
		log_warning( g_logger, "Codigo no reconocido: %d", paquete->codigoOperacion );
		break;
	}

	destruirPaquete(paquete);
	return NULL;
}



void armarConfigMemoria() {
	char* ruta = RUTACONFIG;

	log_info( g_logger, "Leyendo config: %s", ruta );

	g_config = config_create(ruta);
	g_configuracion = malloc( sizeof( t_configuracion ) );

	g_configuracion->puertoConexion    = config_get_string_value(g_config, "LISTEN_PORT");
	g_configuracion->tamanioMemoria    = config_get_int_value(g_config, "MEMORY_SIZE");
	g_configuracion->tamanioSwap    = config_get_int_value(g_config, "PAGE_SIZE");
	g_configuracion->tamanioPagina    = config_get_int_value(g_config, "SWAP_SIZE");

	config_destroy(g_config);
}

void reservarEspacioMemoriaPrincipal(){

	log_debug( g_loggerDebug, "Reservando memoria (bytes) %d", g_configuracion->tamanioMemoria );
	g_granMalloc = malloc( g_configuracion->tamanioMemoria );

	g_cantidadFrames = ( int )( g_configuracion->tamanioMemoria /  g_configuracion->tamanioPagina );
	char * data = malloc( g_cantidadFrames );
	memset(data, 0, g_cantidadFrames); // Inicializo todos los marcos en 0 ( libres
	g_bitarray_marcos = bitarray_create_with_mode(data, g_cantidadFrames, MSB_FIRST);

	maxPaginasEnSwap =  ( int )( g_configuracion->tamanioSwap / g_configuracion->tamanioPagina );
	char * dataSwap = malloc( maxPaginasEnSwap );
	memset(dataSwap, 0, maxPaginasEnSwap); // Inicializo todos los marcos en 0 ( libres )
	g_bitarray_swap = bitarray_create_with_mode(dataSwap, maxPaginasEnSwap, MSB_FIRST);

}

void InicializarNuevoPrograma(int socket){
	log_info( g_logger, "Levanto programa nro %d",nroPrograma);
	t_programa * nuevoPrograma = crearPrograma(socket);
	list_add(programas,nuevoPrograma);

}

void FinalizarPrograma(int socket){
	destruirPrograma(buscarPrograma(socket));
	ActualizarLogMetricas();
}

void destruirGlobales(){}


