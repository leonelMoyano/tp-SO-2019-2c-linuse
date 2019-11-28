#include "MUSE.h"

int main(void) {

	punteroClock = 0;
	nroPrograma = 0;
	direccionamientoLogicoActual = 0;

	g_logger = log_create("MUSE.log", "MUSE", true, LOG_LEVEL_TRACE);
	g_loggerDebug = log_create("MUSE.log", "MUSE", false, LOG_LEVEL_DEBUG);
	log_info( g_logger, "Inicio proceso de MUSE" );

	programas = crearTablaProgramas();

	//abrirArchivoSwap();  va a romper por ruta configurada
	//armarConfigMemoria(); va a romper por ruta configurada;

	iniciarServer(g_configuracion->puertoConexion, procesarPaqueteLibMuse(NULL, LIBMUSE),g_logger);

	return prueba();

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

	log_info( g_logger, "Leyendo config: %s", RUTACONFIG );

	g_config = config_create(RUTACONFIG);
	g_configuracion = malloc( sizeof( g_configuracion ) );

	g_configuracion->puertoConexion    = config_get_int_value(g_config, "LISTEN_PORT");
	g_configuracion->tamanioMemoria    = config_get_int_value(g_config, "MEMORY_SIZE");
	g_configuracion->tamanioSwap    = config_get_int_value(g_config, "PAGE_SIZE");
	g_configuracion->tamanioPagina    = config_get_int_value(g_config, "SWAP_SIZE");


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
	memset(data, 0, maxPaginasEnSwap); // Inicializo todos los marcos en 0 ( libres )
	g_bitarray_swap = bitarray_create_with_mode(data, maxPaginasEnSwap, MSB_FIRST);

}

void InicializarNuevoPrograma(int socket){
	t_programa * nuevoPrograma = crearPrograma(socket);
	list_add(programas,nuevoPrograma);
}

void FinalizarPrograma(int socket){
	destruirPrograma(buscarPrograma(socket));
	ActualizarLogMetricas();
}


void abrirArchivoSwap(char * rutaArchivo, size_t * tamArc, FILE ** archivo) {
	// Abro el archivo
	*archivo = fopen(rutaArchivo, "r");

	if (*archivo == NULL) {
		log_error(g_logger, "%s: No existe el archivo", rutaArchivo);
		exit(EXIT_FAILURE);
	}

	// Copio informacion del archivo
	struct stat statArch;
	stat(rutaArchivo, &statArch);

	// Tamaño del archivo que voy a leer
	*tamArc = g_configuracion->tamanioSwap;

	// Leo el total del archivo y lo asigno al buffer
	void * dataArchivo = calloc( 1, *tamArc + 1 );
	fread( dataArchivo, *tamArc, 1, *archivo );
	log_debug(g_logger, "Abrio el archivo de swap: %s", rutaArchivo);

	//Cierro el archivo ??
	//fclose(*archivo);

	// Hago trim para borrar saltos de linea vacios al final
	string_trim( &( dataArchivo ) );
	disco_swap = dataArchivo;
	//se lo seteo a disco swap??
}

void mapearArchivoMUSE(char * rutaArchivo, size_t * tamArc, FILE ** archivo) {
	// Abro el archivo
	*archivo = fopen(rutaArchivo, "r");

	if (*archivo == NULL) {
		log_error(g_logger, "%s: No existe el archivo", rutaArchivo);
		exit(EXIT_FAILURE);
	}

	// Copio informacion del archivo
	struct stat statArch;
	stat(rutaArchivo, &statArch);

	// Tamaño del archivo que voy a leer
	*tamArc = tamArc;

	// Leo el total del archivo y lo asigno al buffer
	void * dataArchivo = calloc( 1, *tamArc + 1 );
	fread( dataArchivo, *tamArc, 1, *archivo );
	log_debug(g_logger, "Mapeo archivo a MUSE: %s", rutaArchivo);

	//Cierro el archivo ??
	//fclose(*archivo);

	// Hago trim para borrar saltos de linea vacios al final
	string_trim( &( dataArchivo ) );
}




