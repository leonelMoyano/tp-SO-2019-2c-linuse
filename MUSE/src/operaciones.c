#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){

	log_info( g_logger, "Alloqueo algunos bytes");
	t_programa * programa = buscarPrograma(socket);
	t_segmento * segmentoElegido;
	uint32_t direccionLogica = 0;

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
		list_add(programa->segmentos_programa->lista_segmentos,segmentoElegido);
		int auxNoUsar = allocarHeapNuevo(programa , segmentoElegido, tam);
	}
	else
	{
		direccionLogica = allocarEnHeapLibre(tam,programa->segmentos_programa);

		if(direccionLogica == -1)
		{
			t_segmento * ultimoSegmento =  ultimoSegmentoPrograma(programa);
			if(ultimoSegmento->tipoSegmento == 2)//segmento mmap
			{
				segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam);
				list_add(programa->segmentos_programa->lista_segmentos,segmentoElegido);
				programa->segmentos_programa->limiteLogico += tam;

			}
			else{
				segmentoElegido = ultimoSegmento;
				direccionLogica = allocarHeapNuevo(socket,segmentoElegido, tam);
				int cantPaginas = framesNecesariosPorCantidadMemoria(tam);

			}
		}
		else{

			segmentoElegido = buscarSegmento(programa->segmentos_programa->lista_segmentos,direccionLogica);
			int ok = cambiarFramesPorHeap(segmentoElegido, direccionLogica, tam, 1);

		}
	}

	ActualizarLogMetricas();
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
			if(heapLiberar->isFree == false){
				int sizeFreeAgregar = verificarCompactacionFree(segmento->heapsSegmento, indiceHeap);
				//verificar liberacion frames;
				heapLiberar-> isFree = true;
				heapLiberar-> t_size = heapLiberar->t_size + sizeFreeAgregar;
				cambiarFramesPorHeap(segmento,dir,0,0); //TODO: ver si no modifica aca? creo que no
			}
		}
	}
	//si es segmento mmap no debera liberar nada o error?
	//no debo liberar memoria de las paginas ni del frame en cuestion porque es solo free de MV muse?
}

int verificarCompactacionFree(t_list* heaps, int indiceHeap){

	//nose si rompe o devuelve nulo si paso un index mayor o uno menor a 0
	t_heapSegmento * auxHeapAnterior = list_get(heaps, indiceHeap - 1);
	t_heapSegmento * auxHeapPosterior = list_get(heaps, indiceHeap + 1);

	int tamanioAgregar = 0;

	if(auxHeapAnterior->isFree){
		tamanioAgregar += auxHeapAnterior->t_size;
		list_remove_and_destroy_element(heaps,indiceHeap - 1, (void*) destruirHeap);
	}

	if(auxHeapPosterior->isFree){
			tamanioAgregar += auxHeapPosterior->t_size;
			//cambia el indice porque destrui una posicion
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

	return copiarContenidoDeFrames(segmento,src,n,dst);
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
		t_heapSegmento * auxHeap = list_get(segmento->heapsSegmento, indiceHeap);

		if(auxHeap->isFree){
			if(auxHeap->t_size > n){
				auxHeap->isFree = false;
				auxHeap->t_size = n;
				t_heapSegmento* heapHueco = crearHeap(auxHeap->t_size - n,true);
				cambiarFramesPorHeap(segmento,dst,n,true);
			}
			else{
				//voy a pisar otro heap metadata ...que despues va a romper
			}
		}
	}
	else{
		return copiarContenidoAFrames(segmento,dst,n,src);
	}

	return 0;
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
			nuevoSegmento = crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,length,1,mapAbierto);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			mapAbierto->cantProcesosUsando = mapAbierto->cantProcesosUsando + 1;
		}
		else{
			//Mmap compartido nuevo
			mapAbierto = crearMapeo(path,contenidoMap);
			nuevoSegmento = crearSegmentoMmapCompartido(programa->segmentos_programa->limiteLogico,length,0,mapAbierto);
			paginasDeMapASwap(mapAbierto,length,contenidoMap,nuevoSegmento,socket);
			nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
			list_add(mapeosAbiertosCompartidos,mapAbierto);
		}

	}
	else{ //mapeo privado
		mapAbierto = crearMapeo(path,contenidoMap);
		nuevoSegmento = crearSegmentoMmap(programa->segmentos_programa->limiteLogico,length,mapAbierto);
		paginasDeMapASwap(mapAbierto,length,contenidoMap,nuevoSegmento,socket);
		nuevoSegmento->tablaPaginas = mapAbierto->tablaPaginas;
	}

	list_add(programa->segmentos_programa->lista_segmentos,nuevoSegmento);
	programa->segmentos_programa->limiteLogico += length;

	return nuevoSegmento->baseLogica;
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(socket);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,addr);

	if(segmento->tipoSegmento == 2){
		//syncronizar los contenidos del map o levantar el contenido actualizado de los frames de las paginas y volcarlo
		//en una sola posicion de memoria?
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
		int huecoGenerado =  heapBuscado->t_size - cantidadBytesNecesarios;
		if(huecoGenerado > 0){
			t_heapSegmento * heapHueco = crearHeap(huecoGenerado,true);
			list_add_in_index(segmentoBuscar->heapsSegmento, i , heapHueco);
		}

		direccionHeap += tamanio_heap;
	}


	return -1;
}

void allocarEnPaginasNuevas(t_programa* programa, t_segmento* segmentoAExtender, int cantPaginasNecesarias ){
	int i;

	for (i = 0; cantPaginasNecesarias > i ; ++i){
		int indiceFrame = buscarFrameLibre();
		if(indiceFrame == -1) indiceFrame = ClockModificado();
		else
		agregarPaginaEnSegmento(programa->socket, segmentoAExtender,indiceFrame);
	}

	segmentoAExtender->limiteLogico += cantPaginasNecesarias * lengthPagina;
	programa->segmentos_programa->limiteLogico += segmentoAExtender->limiteLogico;	
}

int allocarHeapNuevo(t_programa* programa, t_segmento* segmento, int cantBytesNecesarios){

	uint32_t direccionLogica = segmento->limiteLogico;

	t_heapSegmento* ultimoHeap = list_get(segmento->heapsSegmento, list_size(segmento->heapsSegmento));
	if(ultimoHeap != NULL && ultimoHeap->isFree){
		cantBytesNecesarios = cantBytesNecesarios - ultimoHeap->t_size;
		ultimoHeap->isFree = false;
		direccionLogica = direccionLogica - ultimoHeap->t_size;
		ultimoHeap->t_size = cantBytesNecesarios;
	}
	else{
		t_heapSegmento* heapNuevo = crearHeap(cantBytesNecesarios,false);
		list_add(segmento->heapsSegmento,heapNuevo);
	}
	int cantPaginas = framesNecesariosPorCantidadMemoria(cantBytesNecesarios);
	int huecoLibre = cantBytesNecesarios - (cantPaginas * lengthPagina);
	if(huecoLibre > 0) {
		t_heapSegmento* heapNuevoHueco = crearHeap(huecoLibre,true);
		list_add(segmento->heapsSegmento,heapNuevoHueco);
	}

	allocarEnPaginasNuevas(programa,segmento,cantPaginas);

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


void TraerPaginaDeSwap(int socketPrograma, int nroPagina, int idSegmento){

	int marcoEnSwap = traerFrameDePaginaEnSwap(socketPrograma,idSegmento,nroPagina);
	void* contenido = sacarFrameSwap(marcoEnSwap, &disco_swap);

}

void* sacarFrameSwap(int nroMarco, FILE ** archivo){

	*archivo = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer
	size_t tamArc = g_configuracion->tamanioSwap;

	int fd = fileno(*archivo);

	int indiceArchivo = nroMarco * g_configuracion->tamanioPagina;

	void* dataPagina = mmap(0, lengthPagina, PROT_READ, MAP_SHARED, fd, indiceArchivo);

	//meter pagina en blanco
	void * paginaVacia = mmap( malloc(g_configuracion->tamanioPagina), lengthPagina, PROT_WRITE, MAP_SHARED, fd, indiceArchivo);

	bitarray_clean_bit(g_bitarray_swap,nroMarco);

	fclose(*archivo);

	return dataPagina;
}

void escribirFrameSwap(int nroMarco, void* contenido, FILE ** archivo){

	*archivo = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer
	size_t tamArc = g_configuracion->tamanioSwap;

	// Leo el total del archivo y lo asigno al buffer
	void * dataArchivo = calloc( 1, tamArc + 1 );

	int indiceArchivo = nroMarco * g_configuracion->tamanioPagina;

	int fd = fileno(*archivo);

	void * dataPagina = mmap(0, lengthPagina, PROT_READ, MAP_SHARED, fd, indiceArchivo);
	void * dataPaginaNueva = mmap(contenido, lengthPagina, PROT_WRITE, MAP_SHARED, fd, indiceArchivo);

	bitarray_set_bit(g_bitarray_swap,nroMarco);

	fclose(*archivo);
}

void cargarPaginaEnSwap(void* bytes,int nroPagina, int socketPrograma, int idSegmento){

	int nroFrame = buscarFrameLibreSwap();
	escribirFrameSwap(nroFrame,bytes,&disco_swap);
	list_add(paginasEnSwap, crearPaginaAdministrativa(socketPrograma, idSegmento, nroPagina, nroFrame));

}

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


void paginasDeMapASwap(t_mapAbierto * mapAbierto, size_t tamanioMap, void * contenidoMap,t_segmento* unSegmento,int socket){
	//En el momento de hacer un mapeo dejo la tabla de paginas en swap con el contenido

	int desplazamiento = 0;
	int cantPaginasAMover = framesNecesariosPorCantidadMemoria(tamanioMap);

	for(int i=0;i < cantPaginasAMover;i++){
			t_pagina * paginaNuevo = crearPaginaMap(i);

			int frameSwapElegido = buscarFrameLibreSwap(); //modifico bitaray de swap

			if(frameSwapElegido == -1) perror("Memoria Swap completa"); //es correcto?

			escribirContenidoEnSwap(paginaNuevo,frameSwapElegido,contenidoMap,desplazamiento);

			desplazamiento += g_configuracion->tamanioPagina;

			list_add(paginasEnSwap,crearPaginaAdministrativa(socket,unSegmento->idSegmento,paginaNuevo->nroPagina,0));

			t_paginaAdministrativa* paginaAdmin = buscarPaginaAdministrativaPorPagina(paginasEnSwap,socket,unSegmento->idSegmento,paginaNuevo->nroPagina);

			paginaAdmin->nroFrame = frameSwapElegido; //Asigno el indice de FrameSwap a la paginaAdminist de la pagina

			list_add(mapAbierto->tablaPaginas, paginaNuevo);
		}
}

void escribirContenidoEnSwap(t_pagina * unaPagina,int indiceLibre,void* contenido,int desplazamiento){
	//contenido debe ser una direccion Libre
	int indiceSwap = indiceLibre * g_configuracion->tamanioPagina;

	void * contenidoACopiar = contenido + desplazamiento;

	memcpy(archivoSwap + indiceSwap, contenidoACopiar ,lengthPagina);

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
				modificarPresencia(pag,cargo,0); //TODO: ver si no modifica aca? creo que no
	}

}

int copiarContenidoDeFrames(t_segmento* segmento, uint32_t direccionLogica, size_t tamanio,void* contenidoDestino)
{
	int desplazamiento = 0;
	int nroPaginaInicial = nroPaginaSegmento(direccionLogica, segmento->baseLogica);
	int offsetInicial = desplazamientoPaginaSegmento(direccionLogica, segmento->baseLogica);
	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);

	//tendria que verificar que si el frame esta libre o no tiene contenido
	if(offsetInicial > 0){
		desplazamiento = (g_configuracion->tamanioPagina - offsetInicial);
		tamanio = tamanio - desplazamiento;
		t_pagina* pagina = list_get(segmento->tablaPaginas,nroPaginaInicial);
		if(pagina == NULL) return -1;
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		//if(frame == NULL) return -1; 	//si esta null o no presente ,page fault traer de disco
		memcpy(&contenidoDestino,&frame->contenido[offsetInicial],desplazamiento);
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
		nroPaginaInicial++;
		cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);
	}

	for(int i= nroPaginaInicial; cantPaginasAObtener > i; i++){
		desplazamiento = tamanio > lengthPagina ? lengthPagina: tamanio;
		t_pagina* pagina = list_get(segmento->tablaPaginas,i);
		if(pagina == NULL) return -1; //TODO: poner en todos lados, si no encuentra la pagina rompe???? Preguntar
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		//si esta null o no presente ,page fault traer de disco
		memcpy(&contenidoDestino,&frame->contenido[offsetInicial],desplazamiento);
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
	}

}

int copiarContenidoAFrames(t_segmento* segmento, uint32_t direccionLogica, int tamanio,void* porcionMemoria)
{
	int desplazamiento = 0;
	int nroPaginaInicial = nroPaginaSegmento(direccionLogica, segmento->baseLogica);
	int offsetInicial = desplazamientoPaginaSegmento(direccionLogica, segmento->baseLogica);
	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);


	//tendria que validar que las paginas esten libres, y no haya contenido cargado en el frame?
	if(offsetInicial > 0){
		desplazamiento = (g_configuracion->tamanioPagina - offsetInicial);
		tamanio = tamanio - desplazamiento;
		t_pagina* pagina = list_get(segmento->tablaPaginas,nroPaginaInicial);
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		//si esta null o no presente ,page fault traer de disco
		memcpy( &frame->contenido[offsetInicial], &porcionMemoria[0],desplazamiento);
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
		nroPaginaInicial++;
		cantPaginasAObtener = framesNecesariosPorCantidadMemoria(tamanio);
	}

	for(int i= nroPaginaInicial; cantPaginasAObtener > i; i++){
		desplazamiento = tamanio > lengthPagina ? lengthPagina: tamanio;
		t_pagina* pagina = list_get(segmento->tablaPaginas,i);
		t_contenidoFrame* frame = buscarContenidoFrameMemoria(pagina->nroFrame);
		//si esta null o no presente ,page fault traer de disco
		memcpy(&frame->contenido[0],&porcionMemoria[desplazamiento],desplazamiento);
		offsetInicial += desplazamiento;
		tamanio = tamanio - desplazamiento;
	}

}





