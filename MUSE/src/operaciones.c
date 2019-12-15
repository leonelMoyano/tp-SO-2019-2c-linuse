#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){
	t_programa * programa = buscarPrograma(socket);
	t_segmento * segmentoElegido;
	uint32_t direccionLogica = 0;

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
		direccionLogica = allocarEnPaginasNuevas(socket, segmentoElegido,tam);
	}
	else
	{
		direccionLogica = allocarEnHeapLibre(tam,programa->segmentos_programa);
		if(direccionLogica == 0) //Si es direccion = 0 no encontro y hay que extender el ultimo segmento
		{
			t_segmento * ultimoSegmento =  ultimoSegmentoPrograma(programa);
			if(ultimoSegmento->tipoSegmento == 2)//segmento mmap
			{
				segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
			}
			else segmentoElegido = ultimoSegmento;

			direccionLogica = (socket,segmentoElegido,tam);
		}
	}

	ActualizarLogMetricas();
	return direccionLogica;

}
void procesarFree(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa,dir);

	int nroPagina = nroPaginaSegmento(dir, segmento->baseLogica);
	int offsetPagina = desplazamientoPaginaSegmento(dir, segmento->baseLogica);

	//si es segmento mmap no debera liberar nada o error?

	//int cantPaginasALiberar = framesNecesariosPorCantidadMemoria();

	//Buscar heap metadata a liberar - buscarHeapSegmento

	free(dir);

}

int procesarGet(void* dst, uint32_t src, size_t n, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa,src);

	bool segmentoUnico = segmento->limiteLogico > src + n;

	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(n);
}

int procesarCopy(uint32_t dst, void* src, int n, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa,dst);

	bool esExtendible = esSegmentoExtendible(programa->segmentos_programa, segmento);

	if(dst + n > segmento->limiteLogico && !esExtendible){ return -1;}

}

uint32_t procesarMap(char *path, size_t length, int flags, int socket){
	t_programa * programa= buscarPrograma(socket);

	FILE * archivoMap;

	void* contenidoMap = mapearArchivoMUSE(path,length,&archivoMap,flags);

	//TODO: optimizar funcion, evitar repeticion codigo

	t_segmento * nuevoSegmento;

	t_mapAbierto* mapAbierto = buscarMapeoAbierto(path);

	if(flags == MAP_SHARED) {
		if(mapAbierto != NULL){
			//mmap compartido apuntando a mapeo existente
			nuevoSegmento = crearSegmentoMmap;
			crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,length,1,mapAbierto);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			mapAbierto->cantProcesosUsando = mapAbierto->cantProcesosUsando + 1;
		}
		else{
			//Mmap compartido nuevo
			mapAbierto = crearMapeo(path,contenidoMap);
			nuevoSegmento = crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,length,0,mapAbierto);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			list_add(mapeosAbiertosCompartidos,mapAbierto);
		}

	}
	else{ //mapeo privado
		mapAbierto = crearMapeo(path,contenidoMap);
		nuevoSegmento = crearSegmentoMmap(programa->segmentos_programa->limiteLogico,length,mapAbierto);
		nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
	}
	//Es valido cuando se trata de un nuevo mapeo Privado o un Mapeo de un archivo que nuevo
	int cantPaginas = length / g_configuracion->tamanioPagina;
	//separo contenidomap en paginas y las agrego en la tabla del mapAbierto
	int desplazamiento = 0;
	t_pagina * ultimaPagina;
	for(int i=0;i < cantPaginas;i++){
		t_pagina * paginaNuevo = crearPaginaMap(i);

		int frameElegido = buscarFrameLibre();
		if(frameElegido == -1) frameElegido = ClockModificado();//Uso de clock modificacdo para frameLIbre OK?

		void * paginaAux;

		memcpy(paginaAux,contenidoMap + desplazamiento,g_configuracion->tamanioPagina);
		memcpy(mapAbierto->contenidoArchivoMapeado,contenidoMap + desplazamiento,g_configuracion->tamanioPagina);

		agregarContenido(frameElegido,paginaAux); //TODO IVAN - malloc en contenido frame necesario?

		desplazamiento += g_configuracion->tamanioPagina;//TODO IVAN - rellenar con '0' PADDING
		list_add(mapAbierto->tablaPaginas, paginaNuevo);
	};


	list_add(programa->segmentos_programa,nuevoSegmento);
	programa->segmentos_programa->limiteLogico += length;

	return nuevoSegmento->baseLogica;//direccion logica donde comienza mi mapeo
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento * segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos, addr);
	int resultado;
	if(segmento->mmap != NULL){ //TODO IVAN - Se trata de un mapeo validacion correcta?
		resultado = msync(segmento->mmap->contenidoArchivoMapeado,len,MS_SYNC);
	}
	else resultado = 0;

	return resultado;
	//TODO IVAN - Si len es menor al tamano de la pagina en donde se encuentra se debera incluir toda la pagina en la sincro
}

uint32_t procesarUnMap(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa,dir);
	if(segmento->mmap =! NULL){
		t_mapAbierto * mapeo = segmento->mmap;
		//Destruir paginas de esta forma mapeo->tablaPaginas;
		//munmap(mapeo->contenidoArchivoMapeado);
	}
	return 0;
}

	//TODO: sacar de la lista de mapeos abiertos , mapeosAbiertosCompartidos
	//Verificar si otro proceso no tiene abierto el mismo archivo CONTADOR DE ABIERTOS, liberar la tabla de paginas
	//restar contador de procesos usando mapeo, si llega a cero, liberar las paginas



uint32_t allocarEnHeapLibre(uint32_t cantidadBytesNecesarios, t_segmentos_programa* segmentos){
	t_segmento* segmentoBuscar = NULL;
	t_heapSegmento* auxHeap = NULL;
	t_heapSegmento* heapBuscado = NULL;
	uint32_t direccionHeap = 0;
	bool encontrado = false;
	int i = 0;

	for(int j = 0; j < list_size(segmentos->lista_segmentos) && !encontrado; j++)
	{
		segmentoBuscar = list_get(segmentos->lista_segmentos,j);
		direccionHeap = segmentos->baseLogica;
		if(segmentoBuscar->tipoSegmento == 1)
		{ //es heap, para los mmap tengo que usar heaps igual?
			for (int i = 0; i < list_size(segmentoBuscar->heapsSegmento) && !encontrado; i++) {
				auxHeap = list_get(segmentoBuscar->heapsSegmento,i);
				encontrado = heapBuscado->isFree && heapBuscado->t_size > cantidadBytesNecesarios;
				if(!encontrado) direccionHeap += heapBuscado->t_size + tamanio_heap;
				else heapBuscado = auxHeap;
			}
		}
	}

	if(heapBuscado != NULL)	{
		heapBuscado->isFree = false;
		heapBuscado->t_size = cantidadBytesNecesarios;
		//TODO: agregar nuevo heap libre si sobra espacio, verificar si el anterior o posterior esta free , compactar
		direccionHeap += tamanio_heap;
	}


	return direccionHeap;
}

uint32_t allocarEnPaginasNuevas(int socket, t_segmento* segmentoAExtender, uint32_t cantidadBytesNecesarios ){

	int cantPaginasNecesarias = framesNecesariosPorCantidadMemoria(cantidadBytesNecesarios);

	for (int i = 0; cantPaginasNecesarias > i ; ++i){

		int indiceFrame = buscarFrameLibre();

		//TODO si es de mmap solo debo cargar las paginas en el segmento,sin que esten presentes, flag
		if(indiceFrame == -1) indiceFrame = ClockModificado();
		else
		agregarPaginaEnSegmento(socket, segmentoAExtender,indiceFrame);
	}


	uint32_t direccionHeap = 0;
	return direccionHeap;

}

//TODO, esta funcion podria abstraerla y reutilizarla varias veces, buscar heapValido
int freeDireccionLogicaValida(uint32_t direccionLogica, t_segmento* segmento){


	int direccionLogicaAux = 0;
	bool encontrado = false;
	for (int i = 0; i < list_size(segmento->heapsSegmento) && !encontrado; i++)
	{
		t_heapSegmento* auxHeap = list_get(segmento->heapsSegmento,i);
		encontrado = !auxHeap->isFree && (direccionLogicaAux + tamanio_heap == direccionLogica);
		if(!encontrado) direccionLogicaAux += auxHeap->t_size + tamanio_heap;
		else auxHeap->isFree = true; //fijar si tengo que compactar heaps
	}

	if(!encontrado) return -1; //segmentation fault, TODO: buscar codigo syscall seg fault
}


void RegistrarMetricasPrograma(t_programa* programa){

	int porcentaje = PorcentajeAsignacionMemoria(programa);
	int bytesLibres = EspacioLibre(ultimoSegmentoPrograma(programa));

	log_info( g_logger, "Programa: %d",programa->programaId);
	log_info( g_logger, "Porcentaje de asignación de memoria: %d",porcentaje );
	log_info( g_logger, "Espacio disponible último segmento: %d",bytesLibres );
	log_info( g_logger, "Memoria perdida: %d", programa->memoriaPerdida  );
	log_info( g_logger, "Memoria Liberada: %d", programa->memoriaLiberada  );
	log_info( g_logger, "Memory Leaks: %d", programa->memoryLeaks );


}

void ActualizarLogMetricas(){

	list_iterate(programas, RegistrarMetricasPrograma);
	int cantidadBytes = SistemaMemoriaDisponible();
	log_info( g_logger, "La cantidad de memoria del sistema es de n bytes" );

}

uint32_t EspacioLibre(t_segmento* segmento){
	t_heapSegmento* auxHeap = NULL;
	t_heapSegmento* heapBuscado = NULL;
	uint32_t espacioLibre = 0;

	for (int i = 0; i < list_size(segmento->heapsSegmento); i++) {
		auxHeap = list_get(segmento->heapsSegmento,i);
		if(auxHeap->isFree) espacioLibre += auxHeap->t_size;
	}

	return espacioLibre;
}

int PorcentajeAsignacionMemoria(t_programa* programa){}
int SistemaMemoriaDisponible(){}


void TraerPaginaDeSwap(int socketPrograma, int nroPagina, int idSegmento){

	int marcoEnSwap = traerFrameDePaginaEnSwap(socketPrograma,idSegmento,nroPagina);
	void* contenido = sacarFrameSwap(marcoEnSwap, &disco_swap);

}

void * sacarFrameSwap(int nroMarco, FILE ** archivo){

	*archivo = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer

	int fd = fileno(*archivo);

	int indicePaginaSwap = nroMarco * g_configuracion->tamanioPagina;

	char * indice = stringSwap[indicePaginaSwap];

	void * direccion =escribirContenidoFrameEnMemoria(nroMarco,indice);

	int desplazamiento = 0;

	for(int i= 0; i <= g_configuracion->tamanioPagina;i++){
		stringSwap[indicePaginaSwap + desplazamiento]='\0';
	}

	msync(stringSwap,g_configuracion->tamanioSwap,MS_SYNC);//update al archivo SWAP

	bitarray_clean_bit(g_bitarray_swap,nroMarco);

	fclose(*archivo);

	return direccion;
}

void escribirFrameSwap(int  nroMarco, void* contenido, FILE ** archivo){

	*archivo = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer
	size_t tamArc = g_configuracion->tamanioSwap;

	// Leo el total del archivo y lo asigno al buffer
	//void * dataArchivo = calloc( 1, tamArc + 1 );

	int indicePaginaSwap = nroMarco * g_configuracion->tamanioPagina;

	int fd = fileno(*archivo);

	//void * dataPagina = mmap(0, lengthPagina, PROT_READ, MAP_SHARED, fd, indiceArchivo);
	//void * dataPaginaNueva = mmap(contenido, lengthPagina, PROT_WRITE, MAP_SHARED, fd, indiceArchivo);
	stringSwap = mmap(0,tamArc,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	escribirContenidoFrameEnMemoria(nroMarco,stringSwap[indicePaginaSwap]); //funca?
	msync(stringSwap,g_configuracion->tamanioSwap,MS_SYNC); //unpdate al archivo swap

	bitarray_set_bit(g_bitarray_swap,nroMarco);

	fclose(*archivo);
}

void * escribirContenidoFrameEnMemoria(int  nroMarco, void * dir){
	t_contenidoFrame * contenidoFrame = buscarContenidoFrameMemoria(nroMarco);
	return memcpy(dir,contenidoFrame->contenido,g_configuracion->tamanioPagina);
}

void cargarPaginaEnSwap(void* bytes,int nroPagina, int socketPrograma, int idSegmento){

	int nroFrame = buscarFrameLibreSwap();
	escribirFrameSwap(nroFrame,bytes,&disco_swap);
	list_add(paginasEnSwap, crearPaginaAdministrativa(socketPrograma, idSegmento, nroPagina, nroFrame));
}

void* leerArchivoCompartido(){}

void* escribirEnArchivoCompartido(){}

void * mapearArchivoMUSE(char * rutaArchivo, size_t * tamArc, FILE ** archivo, int flags) {
	//Abro el archivo
	*archivo = fopen(rutaArchivo, "r");

	if (*archivo == NULL) {
		printf("%s: No existe el archivo o el directorio", rutaArchivo);
		return NULL;
	}

	//Copio informacion del archivo
	struct stat statArch;

	stat(rutaArchivo, &statArch);

	//Tamaño del archivo que voy a leer
	*tamArc = statArch.st_size;

	//Leo el total del archivo y lo asigno al buffer
	int fd = fileno(*archivo);
	void * dataArchivo = mmap(0, *tamArc, PROT_READ, flags, fd, 0);

	return dataArchivo;
}






