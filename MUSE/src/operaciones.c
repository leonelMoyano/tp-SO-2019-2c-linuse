#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){

	t_programa * programa = buscarPrograma(socket);
	log_info( g_logger, "Programa %d: Alloqueo %lu bytes",programa->programaId,tam);
	t_segmento * segmentoElegido;
	uint32_t direccionLogica = 5;

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		log_info( g_logger, "Creo el primer segmento del programa %d",programa->programaId);
		segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
		list_add(programa->segmentos_programa->lista_segmentos,segmentoElegido);
		int auxNoUsar = allocarHeapNuevo(programa , segmentoElegido, tam);
	}
	else
	{
		log_info( g_logger, "Busco heap Libre");
		direccionLogica = allocarEnHeapLibre(tam,programa->segmentos_programa);

		if(direccionLogica == -1)
		{
			t_segmento * ultimoSegmento =  ultimoSegmentoPrograma(programa);
			if(ultimoSegmento->tipoSegmento == 2)//segmento mmap
			{
				log_info( g_logger, "Creo nuevo segmento");
				segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
				list_add(programa->segmentos_programa->lista_segmentos,segmentoElegido);
				programa->segmentos_programa->limiteLogico += segmentoElegido->limiteLogico;
				direccionLogica = segmentoElegido->baseLogica;

			}
			else{
				log_info( g_logger, "Redimensiono el ultimo segmento del programa %d",programa->programaId);
				segmentoElegido = ultimoSegmento;
				direccionLogica = allocarHeapNuevo(programa,segmentoElegido, tam);
				int cantPaginas = framesNecesariosPorCantidadMemoria(tam);

			}
		}
		else{
			segmentoElegido = buscarSegmento(programa->segmentos_programa->lista_segmentos,direccionLogica);
			int ok = cambiarFramesPorHeap(segmentoElegido, direccionLogica, tam, 1);
		}
	}

	//ActualizarLogMetricas();
	log_info( g_logger, "Alloqueo en direccion virtual %lu",direccionLogica);
	return direccionLogica;

}
void procesarFree(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,dir);

	if(segmento->tipoSegmento == 1)
	{
		int indiceHeap = esDireccionLogicaValida(dir, segmento);

		if(indiceHeap != -1){
			t_heapSegmento * heapLiberar = list_get(segmento->heapsSegmento, indiceHeap);
			log_info( g_logger, "Programa %d: Libero %d bytes del heap correspondiente a la direccion virtual %lu",programa->programaId,heapLiberar->t_size,dir);
			if(heapLiberar->isFree == false){
				int sizeFreeAgregar = verificarCompactacionFree(segmento->heapsSegmento, indiceHeap);
				//verificar liberacion frames;
				heapLiberar-> isFree = true;
				heapLiberar-> t_size = heapLiberar->t_size + sizeFreeAgregar;
				cambiarFramesPorHeap(segmento,dir,0,0); //TODO: ver si no modifica aca? creo que no
			}
		}
	}

}

int verificarCompactacionFree(t_list* heaps, int indiceHeap){

	//nose si rompe o devuelve nulo si paso un index mayor o uno menor a 0
	t_heapSegmento * auxHeapAnterior = list_get(heaps, indiceHeap - 1);
	t_heapSegmento * auxHeapPosterior = list_get(heaps, indiceHeap + 1);

	int tamanioAgregar = 0;

	if(auxHeapAnterior->isFree){
		tamanioAgregar += auxHeapAnterior->t_size + tamanio_heap;
		log_info( g_logger, "Compacto %lu bytes del heap anterior más los 5 bytes de la metadata",auxHeapAnterior->t_size);
		list_remove_and_destroy_element(heaps,indiceHeap - 1, (void*) destruirHeap);
	}

	if(auxHeapPosterior->isFree){
			tamanioAgregar += auxHeapPosterior->t_size + tamanio_heap;
			//cambia el indice porque destrui una posicion
			log_info( g_logger, "Compacto %lu bytes del heap posterior más los 5 bytes de la metadata",auxHeapAnterior->t_size);
			list_remove_and_destroy_element(heaps,indiceHeap, (void*) destruirHeap);
	}

	return tamanioAgregar;
}

int procesarGet(void* dst, uint32_t src, size_t n, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,src);
	if(segmento == NULL) return -1;

	bool segmentoUnico = segmento->limiteLogico > src + n;
	if(!segmentoUnico) return -1;

	return copiarContenidoDeFrames(socket,segmento,src,n,dst);
}

int procesarCopy(uint32_t dst, void* src, int n, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,dst);
	if(segmento == NULL) return -1;

	bool segmentoUnico = segmento->limiteLogico > dst + n;
	if(!segmentoUnico) return -1;

	bool esExtendible = esSegmentoExtendible(programa->segmentos_programa, segmento);
	if(dst + n > segmento->limiteLogico){ return -1;}

	if(segmento->tipoSegmento == 1){
		int indiceHeap = esDireccionLogicaValida(dst,segmento);
		if(indiceHeap == -1) return indiceHeap;
		t_heapSegmento * auxHeap = list_get(segmento->heapsSegmento, indiceHeap);

		log_info( g_logger, "Programa %d: Copio %d bytes a direcion virtual %lu",programa->programaId,n,dst);

		auxHeap->isFree = false;
		if(auxHeap->t_size > n){
			t_heapSegmento* heapHueco = crearHeap(auxHeap->t_size - n,true);
			list_add(segmento->heapsSegmento,heapHueco);
			cambiarFramesPorHeap(segmento,dst,n,true);
			copiarContenidoAFrames(socket,segmento,dst,n,src);
		}
		else{
			int tamanioAuxiliar = n - auxHeap->t_size;
			while(tamanioAuxiliar  > 0){
				t_heapSegmento * auxHeapAPisar = list_get(segmento->heapsSegmento, indiceHeap);
				tamanioAuxiliar = tamanioAuxiliar - (auxHeapAPisar->t_size + tamanio_heap);
				list_remove_and_destroy_element(segmento->heapsSegmento,indiceHeap,destruirHeap);
				//TODO: ver de manejar un t_heapMetadata negativo para saber que es invalido pero contar ese tamanio como
				//numeracion de las direcioneslogicas
			}
			if(0  > tamanioAuxiliar){
				t_heapSegmento* huecoInvalido = crearHeap(tamanioAuxiliar,false);
				list_add(segmento->heapsSegmento,huecoInvalido);
			}


		cambiarFramesPorHeap(segmento,dst,n,true);
		copiarContenidoAFrames(socket,segmento,dst,n,src);
		}

	}
	else{
		return copiarContenidoAFrames(socket,segmento,dst,n,src);
	}

	return 0;
}

uint32_t procesarMap(char *path, size_t length, int flags, int socket){
	t_programa * programa= buscarPrograma(socket);

	FILE * archivoMap;

	//TODO: revisar este map y el pasaje de los flags
	void* contenidoMap = mapearArchivoMUSE(path,&length,&archivoMap,flags);

	//TODO: optimizar funcion, evitar repeticion codigo

	t_segmento * nuevoSegmento;

	t_mapAbierto* mapAbierto = buscarMapeoAbierto(path);

	int cantidadPaginas = framesNecesariosPorCantidadMemoria(length);
	int tamanioLogico = cantidadPaginas * lengthPagina;
	if(flags == MAP_SHARED) {
		if(mapAbierto != NULL){
			//mmap compartido apuntando a mapeo existente
			nuevoSegmento = crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,tamanioLogico,1,mapAbierto);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			mapAbierto->cantProcesosUsando = mapAbierto->cantProcesosUsando + 1;
		}
		else{
			//Mmap compartido nuevo
			mapAbierto = crearMapeo(path,contenidoMap);
			nuevoSegmento = crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,tamanioLogico,0,mapAbierto);
			paginasDeMapASwap(mapAbierto,length,contenidoMap,nuevoSegmento,socket);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			list_add(mapeosAbiertosCompartidos,mapAbierto);
		}

	}
	else{ //mapeo privado
		mapAbierto = crearMapeo(path,contenidoMap);
		nuevoSegmento = crearSegmentoMmap(programa->segmentos_programa->limiteLogico,tamanioLogico,mapAbierto);
		paginasDeMapASwap(mapAbierto,length,contenidoMap,nuevoSegmento,socket);
		nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
	}

	list_add(programa->segmentos_programa->lista_segmentos,nuevoSegmento);
	programa->segmentos_programa->limiteLogico += tamanioLogico;

	//  TODO revisar caso
	// el mmap caiga en un hueco al principio por lo que el limite logico no se modifica

	return nuevoSegmento->baseLogica;
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,addr);

	if(segmento->tipoSegmento == 2){
		void* archivoActualizado;
		copiarContenidoDeFrames(socket,segmento,addr,len,archivoActualizado);
		memcpy(segmento->mmap->contenido,archivoActualizado,len);
		msync(segmento->mmap->contenido,len,MS_SYNC);
	}

	return 0;

}

uint32_t procesarUnMap(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,dir);

	if(segmento == NULL || segmento->tipoSegmento == 1 || dir != segmento->baseLogica ) return -1;

	//me parece que asi no contempla el verdadero largo del void*
	int largoArchivo = segmento->limiteLogico - segmento->baseLogica;

	if(segmento->mmap->cantProcesosUsando == 1){
		borrarMapeoAbierto(segmento->mmap->path); //ver de usar id de segmento
		destruirSegmentoMap(segmento,1);
		munmap(segmento->mmap->contenido,largoArchivo);
	}
	else{
		segmento->mmap->cantProcesosUsando = segmento->mmap->cantProcesosUsando - 1;
		destruirSegmentoMap(segmento,0);
	}

	return 0;
}


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
		direccionHeap = segmentos->baseLogica + tamanio_heap;
		if(segmentoBuscar->tipoSegmento == 1)
		{
			for (int i = 0; i < list_size(segmentoBuscar->heapsSegmento) && !encontrado; i++) {
				auxHeap = list_get(segmentoBuscar->heapsSegmento,i);
				encontrado = auxHeap->isFree && auxHeap->t_size >= cantidadBytesNecesarios;
				if(!encontrado) direccionHeap += auxHeap->t_size + tamanio_heap;
				else {
					heapBuscado = auxHeap;
					heapBuscado->isFree = false;
				}
			}
		}
	}

	if(heapBuscado != NULL)	{
		int huecoGenerado =  heapBuscado->t_size - cantidadBytesNecesarios;
		heapBuscado->t_size = cantidadBytesNecesarios;
		if(huecoGenerado > 5){
			huecoGenerado = huecoGenerado - tamanio_heap;
			t_heapSegmento * heapHueco = crearHeap(huecoGenerado,true);
			list_add(segmentoBuscar->heapsSegmento,heapHueco);
		}
		return direccionHeap;
	}
	else return -1;
}

int allocarEnPaginasNuevas(t_programa* programa, t_segmento* segmentoAExtender, int cantPaginasNecesarias ){
	int indiceNuevaPagina = list_size(segmentoAExtender->tablaPaginas) - 1;
	for (int i = 0; cantPaginasNecesarias > i ; ++i){
		int indiceFrame = buscarFrameLibre();
		if(indiceFrame == -1) indiceFrame = ClockModificado();
		else
		agregarPaginaEnSegmento(programa->socket, segmentoAExtender,indiceFrame);
	}

	segmentoAExtender->limiteLogico += cantPaginasNecesarias * lengthPagina;
	programa->segmentos_programa->limiteLogico += segmentoAExtender->limiteLogico;	
	return indiceNuevaPagina + 1;
}

int allocarHeapNuevo(t_programa* programa, t_segmento* segmento, int cantBytesNecesarios){

	int espacioLibreUltimaPagina = huecoUltimaPagina(segmento);
	uint32_t direccionLogica = segmento->baseLogica + tamanio_heap;

	t_heapSegmento* ultimoHeap = list_get(segmento->heapsSegmento, list_size(segmento->heapsSegmento) - 1);
	if(ultimoHeap != NULL && ultimoHeap->isFree){
		cantBytesNecesarios = cantBytesNecesarios - ultimoHeap->t_size;
		ultimoHeap->isFree = false;
		direccionLogica = direccionLogica + ultimoHeap->t_size;
		ultimoHeap->t_size = cantBytesNecesarios;
	}
	else{
		t_heapSegmento* heapNuevo = crearHeap(cantBytesNecesarios,false);
		list_add(segmento->heapsSegmento,heapNuevo);
	}
	int bytesNecesarios = cantBytesNecesarios  + tamanio_heap - espacioLibreUltimaPagina;
	int cantPaginas = framesNecesariosPorCantidadMemoria(cantBytesNecesarios);
	int huecoLibre = (cantPaginas * lengthPagina) - bytesNecesarios;
	if(huecoLibre > 5) {
		t_heapSegmento* heapNuevoHueco = crearHeap(huecoLibre - tamanio_heap,true);
		list_add(segmento->heapsSegmento,heapNuevoHueco);
	}

	int indicePrimeraPagina = allocarEnPaginasNuevas(programa,segmento,cantPaginas);
	int tamanioHeapAux = tamanio_heap - espacioLibreUltimaPagina;
	direccionLogica = (indicePrimeraPagina * lengthPagina) + tamanioHeapAux;

	return direccionLogica;
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

void TraerPaginaDeSwap(int socketPrograma, t_pagina* pagina, int idSegmento){
	t_paginaAdministrativa* paginaAdmin = buscarPaginaAdministrativaPorPagina(paginasEnSwap,socketPrograma,idSegmento,pagina->nroPagina);
	void* dataPagina = traerContenidoSwap(paginaAdmin->nroFrame);
	int nroFrameMemoria = buscarFrameLibre();
	if(nroFrameMemoria == -1) nroFrameMemoria = ClockModificado();
	agregarContenido(nroFrameMemoria,dataPagina);	
	pagina->nroFrame = nroFrameMemoria;
	modificarPresencia(pagina,true,false);

	sem_wait(&g_mutexSwap);
	borrarPaginaAdministrativaPorFrame(paginasEnSwap,paginaAdmin->nroFrame);
	sem_post(&g_mutexSwap);

	paginaAdmin->nroFrame = nroFrameMemoria;
	list_add(tablasDePaginas,paginaAdmin);	
}

void cargarPaginaEnSwap(void* bytes,int nroPagina, int socketPrograma, int idSegmento){

	int nroFrame = buscarFrameLibreSwap();
	int desplazamiento = nroFrame * g_configuracion->tamanioPagina;
	sem_wait(&g_mutexSwap);
	escribirContenidoEnSwap(nroFrame,bytes,desplazamiento);
	list_add(paginasEnSwap, crearPaginaAdministrativa(socketPrograma, idSegmento, nroPagina, nroFrame));
	sem_post(&g_mutexSwap);

}

void paginasDeMapASwap(t_mapAbierto * mapAbierto, size_t tamanioMap, void * contenidoMap,t_segmento* unSegmento,int socket){
	//En el momento de hacer un mapeo dejo la tabla de paginas en swap con el contenido

	int desplazamiento = 0;
	int cantPaginasAMover = framesNecesariosPorCantidadMemoria(tamanioMap);

	for(int i=0;i < cantPaginasAMover;i++){

			int frameSwapElegido = buscarFrameLibreSwap(); //modifico bitaray de swap

			t_pagina * paginaNuevo = crearPaginaMap(frameSwapElegido, i);

			if(frameSwapElegido == -1) perror("Memoria Swap completa"); //es correcto? si pero en teoria nunca va a pasar

			escribirContenidoEnSwap(frameSwapElegido,contenidoMap,desplazamiento);

			desplazamiento += g_configuracion->tamanioPagina;

			list_add( paginasEnSwap, crearPaginaAdministrativa( socket, unSegmento->idSegmento, paginaNuevo->nroPagina, frameSwapElegido ) );

			t_paginaAdministrativa* paginaAdmin = buscarPaginaAdministrativaPorPagina(paginasEnSwap,socket,unSegmento->idSegmento,paginaNuevo->nroPagina);

			paginaAdmin->nroFrame = frameSwapElegido; //Asigno el indice de FrameSwap a la paginaAdminist de la pagina

			list_add(mapAbierto->tablaPaginas, paginaNuevo);
		}
}

void escribirContenidoEnSwap(int indiceLibre,void* contenido,int desplazamiento){
	//contenido debe ser una direccion Libre
	int indiceSwap = indiceLibre * g_configuracion->tamanioPagina;

	void * contenidoACopiar = contenido + desplazamiento;

	memcpy(g_archivo_swap + indiceSwap, contenidoACopiar ,lengthPagina);

}

void* traerContenidoSwap(int indiceBuscado){
	//contenido debe ser una direccion Libre
	void* memoriaDestino = malloc(lengthPagina);
	int indiceSwap = indiceBuscado * g_configuracion->tamanioPagina;
	memcpy(memoriaDestino, g_archivo_swap + indiceSwap ,lengthPagina);

}


int cambiarFramesPorHeap(t_segmento* segmento, uint32_t direccionLogica, uint32_t tamanio, bool cargo) //el bool es para cargar u ocupar
{
	int desplazamiento = 0;
	int nroPaginaInicial = nroPaginaSegmento(direccionLogica, segmento->baseLogica);
	int offsetInicial = desplazamientoPaginaSegmento(direccionLogica, segmento->baseLogica);
	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);

	if(offsetInicial > 0){
		desplazamiento = (g_configuracion->tamanioPagina - offsetInicial);
		tamanio = tamanio - desplazamiento;
		nroPaginaInicial++;
		cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);
		// si este de abajo es mayor a 0 y menor a la pagina, la ultima pagina no la ocupo
		if(bytesNecesariosUltimoFrame(tamanio) != 0) cantPaginasAObtener = cantPaginasAObtener - 1;
	}

	for(int i= nroPaginaInicial; cantPaginasAObtener > i; i++){
				t_pagina* pag = list_get(segmento->tablaPaginas,i);
				//tengo que reservar frame no?
				modificarPresencia(pag,cargo,0); //TODO: ver si no modifica aca? creo que no
	}

	return 0;
}

int copiarContenidoDeFrames(int socket,t_segmento* segmento, uint32_t direccionLogica, size_t tamanio,void* contenidoDestino)
{
	int desplazamiento = 0;
	int nroPaginaInicial = nroPaginaSegmento(direccionLogica, segmento->baseLogica);
	int offsetInicial = desplazamientoPaginaSegmento(direccionLogica, segmento->baseLogica);
	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);

	//tendria que verificar que si el frame esta libre o no tiene contenido
	if(offsetInicial > 0){
		int restoPagina = (g_configuracion->tamanioPagina - offsetInicial);
		desplazamiento = tamanio > restoPagina ? restoPagina : tamanio;
		tamanio = tamanio - desplazamiento;
		int ok = pageFault(segmento,nroPaginaInicial,contenidoDestino,offsetInicial,desplazamiento,false);
		if(ok == -1) return ok;
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
		nroPaginaInicial++;
		cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);
	}

	for(int i= nroPaginaInicial; cantPaginasAObtener > i; i++){
		desplazamiento = tamanio > lengthPagina ? lengthPagina: tamanio;
		int ok = pageFault(segmento,i,contenidoDestino,offsetInicial,desplazamiento,false);
		if(ok == -1) return ok;
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
	}

}

int copiarContenidoAFrames(int socket,t_segmento* segmento, uint32_t direccionLogica, int tamanio,void* porcionMemoria)
{
	int desplazamiento = 0;
	int nroPaginaInicial = nroPaginaSegmento(direccionLogica, segmento->baseLogica);
	int offsetInicial = desplazamientoPaginaSegmento(direccionLogica, segmento->baseLogica);
	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);

	//tendria que validar que las paginas esten libres, y no haya contenido cargado en el frame?
	if(offsetInicial > 0){
		int restoPagina = (g_configuracion->tamanioPagina - offsetInicial);
		desplazamiento = tamanio > restoPagina ? restoPagina : tamanio;
		tamanio = tamanio - desplazamiento;
		int ok = pageFault(segmento,nroPaginaInicial,porcionMemoria,offsetInicial,desplazamiento,true);
		if(ok == -1) return ok;
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
		nroPaginaInicial++;
		cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);
	}

	for(int i= nroPaginaInicial; cantPaginasAObtener > i; i++){
		desplazamiento = tamanio > lengthPagina ? lengthPagina: tamanio;
		int ok = pageFault(segmento,0,porcionMemoria,offsetInicial,desplazamiento,true);
		if(ok == -1) return ok;
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
	}

}

int pageFault(t_segmento* segmento, int i , void* contenidoDestinoOsrc, int offsetInicial, int desplazamiento, bool operacionInversa){

	t_pagina* pagina = list_get(segmento->tablaPaginas,i);
	if(pagina == NULL) return -1;
	if(!operacionInversa && pagina->nroFrame == NULL) return -1; //verificar esto, pagina sin data y que no esta en swap
	if(!pagina->flagPresencia){
		if(segmento->esCompartido){
			sem_wait(&segmento->mmap->semaforoPaginas);
			TraerPaginaDeSwap(socket,pagina,segmento->idSegmento);
			sem_post(&segmento->mmap->semaforoPaginas);
		}
		else TraerPaginaDeSwap(socket,pagina,segmento->idSegmento);
	}
	//sem_wait(&g_mutexgContenidoFrames);
	if(operacionInversa) {
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		if(frame == NULL){
			log_info( g_logger, "Cargo nuevo frame al segmento");
			void* contenidoFrame = malloc(lengthPagina);
			memcpy(contenidoFrame, contenidoDestinoOsrc + desplazamiento,desplazamiento);
			agregarContenido(pagina->nroFrame,contenidoFrame);
		}
		else{
			log_info( g_logger, "Copio %d bytes del segmento %d - pagina %d - frame %d , desde el offset %d",desplazamiento,segmento->idSegmento,i,pagina->nroFrame,offsetInicial);
			memcpy(frame->contenido, contenidoDestinoOsrc + desplazamiento,desplazamiento);
		}
	}
	else {
		log_info( g_logger, "Obtengo %d bytes del segmento %d - pagina %d - frame %d , desde el offset %d",desplazamiento,segmento->idSegmento,i,pagina->nroFrame,offsetInicial);
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		if(frame == NULL) return -1;
		memcpy(contenidoDestinoOsrc,frame->contenido + offsetInicial,desplazamiento);
	}
	//sem_post(&g_mutexgContenidoFrames);
}

void cargarFrameASwap(int nroFrame, t_paginaAdministrativa * paginaAdmin){

	t_contenidoFrame * miFrame = buscarContenidoFrameMemoria(nroFrame);

	void * contenido = miFrame->contenido;

	int indiceFrame = buscarFrameLibreSwap() * g_configuracion->tamanioPagina;

	sem_wait(&g_mutexgBitarray_marcos);
	bitarray_clean_bit(g_bitarray_marcos,nroFrame);
	sem_post(&g_mutexgBitarray_marcos);

	sem_wait(&g_mutexSwap);
	memcpy(g_archivo_swap + indiceFrame, contenido, lengthPagina); //copio a swap mapeado
	msync(g_archivo_swap,g_configuracion->tamanioSwap,MS_SYNC); // update de mapeo a archivo
	sem_wait(&g_mutexSwap);

	paginaAdmin->nroFrame = indiceFrame; //guardo el indice donde esta la pagina en SWAP

	sem_wait(&g_mutexPaginasEnSwap);
	list_add(paginasEnSwap,paginaAdmin);
	sem_wait(&g_mutexPaginasEnSwap);

}





