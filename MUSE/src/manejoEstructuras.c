#include "manejoEstructuras.h"


t_list* crearDiccionarioConexiones() {
	t_list* aux = list_create();
    //TODO: conexiones de los programas?
	return aux;
}

t_list* crearTablaPaginas() {
	t_list* aux = list_create();
	return aux;
}

t_list* crearListaHeapsMetadata() {
	t_list* aux = list_create();
	return aux;
}

t_segmentos_programa* crearSegmentosPrograma() {
	t_segmentos_programa* aux = malloc(sizeof(t_segmentos_programa));
	aux->baseLogica = 0;
	aux->limiteLogico = 0;
	aux->lista_segmentos = crearTablaSegmentos();
	return aux;
}

t_list* crearTablaSegmentos() {
	t_list* aux = list_create();
	return aux;
}

t_list* crearTablaProgramas() {
	t_list* aux = list_create();
	return aux;
}

t_list* crearListaMapeos() {
	t_list* aux = list_create();
	return aux;
}

t_list* crearListaPaginasSwap() {
	t_list* aux = list_create();
	return aux;
}

t_segmento* crearSegmento(int direccionBase, int tamanio){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	segmentoNuevo->tablaPaginas = crearTablaPaginas();
	segmentoNuevo->baseLogica = direccionBase;
	segmentoNuevo->limiteLogico = tamanio;
	segmentoNuevo->idSegmento = idSegmento;
	segmentoNuevo->tipoSegmento = 1;
	segmentoNuevo->heapsSegmento = crearListaHeapsMetadata();
	segmentoNuevo->esCompartido = false;
	idSegmento++;
	return segmentoNuevo;
}

t_segmento* crearSegmentoMmap(int direccionBase, int tamanio, t_mapAbierto* mapeo ){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	segmentoNuevo->baseLogica = direccionBase;
	segmentoNuevo->limiteLogico = tamanio;
	segmentoNuevo->idSegmento = idSegmento;
	segmentoNuevo->tipoSegmento = 2;
	segmentoNuevo->mmap = mapeo;
	segmentoNuevo->esCompartido = false;
	idSegmento++;
	return segmentoNuevo;
}

t_segmento* crearSegmentoMmapCompartido(int direccionBase, int tamanio, bool tablaDePaginasCompartida, t_mapAbierto* mapeo){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	if(!tablaDePaginasCompartida) segmentoNuevo->tablaPaginas = crearTablaPaginas();
	segmentoNuevo->baseLogica = direccionBase;
	segmentoNuevo->limiteLogico = tamanio;
	segmentoNuevo->idSegmento = idSegmento;
	segmentoNuevo->tipoSegmento = 2;
	segmentoNuevo->mmap = mapeo;
	segmentoNuevo->esCompartido = true;
	idSegmento++;
	return segmentoNuevo;
}

t_programa* crearPrograma(int socket){
	t_programa* programaNuevo = malloc( sizeof( t_programa ) );
	programaNuevo->programaId = nroPrograma;
	programaNuevo->segmentos_programa = crearSegmentosPrograma();
	programaNuevo->socket = socket;
	nroPrograma++;
	return programaNuevo;
}


t_pagina* crearPagina(int nroFrame, int nroPagina){
	t_pagina* pagina = malloc( sizeof( t_pagina ) );
	pagina->flagPresencia  = true; //TODO si es mmap deberia ir en false, agregar flag
	pagina->flagModificado = false;
	pagina->nroFrame  = nroFrame;
	pagina->nroPagina = nroPagina;
	return pagina;
}

t_pagina* crearPaginaMap(int nroFrame, int nroPagina){
	t_pagina* pagina = malloc( sizeof( t_pagina ) );
	pagina->flagPresencia  = false;
	pagina->flagModificado = false;
	// pagina->nroFrame  = NULL;
	pagina->nroFrame  = nroFrame; // inicialmente va a ser el frame en swap
	pagina->nroPagina = nroPagina;
	return pagina;
}

t_paginaAdministrativa* crearPaginaAdministrativa(int socketPrograma, int idSegmento,int nroPagina, int nroFrame){
	t_paginaAdministrativa* paginaAdministrativa = malloc( sizeof( t_paginaAdministrativa ) );
	paginaAdministrativa->socketPrograma  = socketPrograma;
	paginaAdministrativa->idSegmento  = idSegmento;
	paginaAdministrativa->nroPagina   = nroPagina;
	paginaAdministrativa->nroFrame = nroFrame;
	return paginaAdministrativa;
}



void agregarContenido(int nroFrame, void* contenido){
	t_contenidoFrame* contenidoFrame = malloc( sizeof( t_contenidoFrame ) );
	contenidoFrame->nroFrame = nroFrame;
	contenidoFrame->contenido = contenido;
	sem_wait(&g_mutexgContenidoFrames);
	list_add(contenidoFrames, contenidoFrame);
	sem_post(&g_mutexgContenidoFrames);
}

//TODO:agregar map, no compartido, no lo va a agregar a la lista de archivos compartido

t_mapAbierto* crearMapeo(char* path, void* contenido){
	t_mapAbierto* map = malloc(sizeof(t_mapAbierto));
	map->path = path;
	map->tablaPaginas = crearTablaPaginas();
	map->contenido = contenido;
	sem_init(&map->semaforoPaginas, 0, 1);
	map->cantProcesosUsando = 1;
	return map;
}


t_heapSegmento* crearHeap(uint32_t tamanio, bool isFree){
	t_heapSegmento* heap = malloc( sizeof( t_heapSegmento ) );
	heap->isFree  = isFree;
	heap->t_size = tamanio;
	return heap;
}

void agregarPaginaEnSegmento(int socket, t_segmento * segmento, int numeroDeMarco) {
	t_pagina * paginaNuevo = crearPagina( numeroDeMarco, list_size( segmento->tablaPaginas) );
	//TODO:sincronizar todo esto
	bitarray_set_bit( g_bitarray_marcos, numeroDeMarco );
	list_add( segmento->tablaPaginas, paginaNuevo );
	void* contenidoFrame = malloc(lengthPagina);
	agregarContenido(numeroDeMarco, contenidoFrame);
	list_add(tablasDePaginas, crearPaginaAdministrativa(socket,segmento->idSegmento,list_size(segmento->tablaPaginas),numeroDeMarco));
}

t_segmento* buscarSegmento(t_list* segmentos,uint32_t direccionVirtual) {

	bool existeDireccionSegmento(void* segmento){
		t_segmento* segmentoBuscar = (t_segmento*) segmento;

		if (direccionVirtual != NULL) return segmentoBuscar->baseLogica < direccionVirtual   && direccionVirtual  < (  (segmentoBuscar->baseLogica + segmentoBuscar->limiteLogico) );
		return false;

	}
	t_segmento* segmentoBuscado = list_find(segmentos,existeDireccionSegmento);
	return segmentoBuscado;
}

t_mapAbierto* buscarMapeoAbierto(char* path) {

	bool existePath(void* mapAbierto){
		t_mapAbierto* mapAbiertoBuscar = (t_mapAbierto*) mapAbierto;

		if (path != NULL) return string_equals_ignore_case(mapAbiertoBuscar->path, path);
		return false;

	}

	sem_wait(&g_mutexMapeosAbiertosCompartidos);
	t_mapAbierto* mapAbiertoBuscado = list_find(mapeosAbiertosCompartidos,existePath);
	sem_post(&g_mutexMapeosAbiertosCompartidos);
	return mapAbiertoBuscado;
}

void borrarMapeoAbierto(char* path) {

	bool existePath(void* mapAbierto){
		t_mapAbierto* mapAbiertoBuscar = (t_mapAbierto*) mapAbierto;

		if (path != NULL) return string_equals_ignore_case(mapAbiertoBuscar->path, path);
		return false;

	}

	sem_wait(&g_mutexMapeosAbiertosCompartidos);
	list_remove_by_condition(mapeosAbiertosCompartidos,existePath);
	sem_post(&g_mutexMapeosAbiertosCompartidos);
}



t_segmento* buscarSegmentoId(t_list* segmentos,int idSemgneto) {

	bool existeDireccionSegmento(void* segmento){
		t_segmento* segmentoBuscar = (t_segmento*) segmento;

		if (idSemgneto != NULL) return segmentoBuscar->idSegmento == idSemgneto;
		return false;

	}

	t_segmento* segmentoBuscado = list_find(segmentos,existeDireccionSegmento);

	return segmentoBuscado;
}


t_contenidoFrame* buscarContenidoFrameMemoria(int nroFrame) {

	bool existeContenidoFrame(void* contenidoFrame){
		t_contenidoFrame* contenidoBuscar = (t_contenidoFrame*) contenidoFrame;

		if(nroFrame != NULL) return contenidoBuscar->nroFrame == nroFrame;
		return false;

	}

	//sem_wait(&g_mutexgContenidoFrames);
	t_contenidoFrame* contenidoBuscado = list_find(contenidoFrames,existeContenidoFrame);
	//sem_post(&g_mutexgContenidoFrames);
	return contenidoBuscado;
}

int traerFrameDePaginaEnSwap(int socketPrograma,int idSegmento, int nroPagina) {

	bool existeFrame(void* frame){
		t_paginaAdministrativa* paginaBuscar = (t_paginaAdministrativa*) frame;
		if (nroPagina != NULL) return paginaBuscar->nroPagina == nroPagina && paginaBuscar->socketPrograma == socketPrograma && paginaBuscar->idSegmento == idSegmento;
		return false;
	}

	sem_wait(&g_mutexSwap);
	t_paginaAdministrativa* paginaBuscada = list_find(paginasEnSwap,existeFrame);
	sem_post(&g_mutexSwap);
	return paginaBuscada->nroFrame;
}


void borrarPaginaAdministrativaPorFrame(t_list* SwapOPrincipal, int nroFrameSwapOPrincipal){

	bool existeFrame(void* frame){
		t_paginaAdministrativa* frameBuscar = (t_paginaAdministrativa*) frame;

		if (nroFrameSwapOPrincipal != NULL) return frameBuscar->nroFrame == nroFrameSwapOPrincipal;
		return false;
	}
	list_remove_by_condition(SwapOPrincipal,existeFrame);

}

t_paginaAdministrativa* buscarPaginaAdministrativaPorFrame(t_list* SwapOPrincipal, int nroFrameSwapOPrincipal){

	bool existeFrame(void* frame){
		t_paginaAdministrativa* frameBuscar = (t_paginaAdministrativa*) frame;

		if (nroFrameSwapOPrincipal != NULL) return frameBuscar->nroFrame == nroFrameSwapOPrincipal;
		return false;
	}
	t_paginaAdministrativa* paginaAdministrativa = list_find(SwapOPrincipal,existeFrame);

	return paginaAdministrativa;
}

t_paginaAdministrativa* buscarPaginaAdministrativaPorPagina(t_list* SwapOPrincipal, int socketPrograma, int idSegmento, int nroPagina){

	bool existeFrame(void* frame){
		t_paginaAdministrativa* frameBuscar = (t_paginaAdministrativa*) frame;

		if (socketPrograma != NULL && idSegmento != NULL && nroPagina != NULL)
			return frameBuscar->idSegmento == idSegmento && frameBuscar->socketPrograma == socketPrograma && frameBuscar->nroPagina == nroPagina ;
		return false;
	}
	t_paginaAdministrativa* paginaAdministrativa = list_find(SwapOPrincipal,existeFrame);

	return paginaAdministrativa;
}

t_pagina* buscarFrameEnTablasDePaginas(t_paginaAdministrativa* paginaABuscar) {

	t_programa * programa= buscarPrograma(paginaABuscar->socketPrograma);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa->lista_segmentos,paginaABuscar->idSegmento);
	t_pagina* paginaBuscada = list_get(segmento->tablaPaginas,paginaABuscar->nroPagina);
	return paginaBuscada;
}


t_programa* buscarPrograma(int socket) {

	bool existeIdPrograma(void* programa){
		t_programa* programaBuscar = (t_programa*) programa;

		if (socket != NULL) return programaBuscar->socket == socket;
		return false;

	}

	sem_wait(&g_mutexTablaProgramas);
	t_programa* programaBuscado = list_find(programas,existeIdPrograma);
	sem_post(&g_mutexTablaProgramas);
	return programaBuscado;
}

bool esSegmentoExtendible(t_segmentos_programa* segmentos, t_segmento* segmento){

	return segmentos->limiteLogico > segmento->limiteLogico;
}

int nroPaginaSegmento(uint32_t direccionVirtual, int baseLogica){
	return (direccionVirtual - baseLogica) / g_configuracion->tamanioPagina ;
}

int desplazamientoPaginaSegmento(uint32_t direccionVirtual, int baseLogica){
	int nroPagina = nroPaginaSegmento(direccionVirtual,baseLogica);
	return (direccionVirtual - baseLogica) - (nroPagina * g_configuracion->tamanioPagina);
}

int buscarFrameLibre(){
	for (int i = 0; i < g_cantidadFrames; i++) {
		sem_wait(&g_mutexgBitarray_marcos);
		if( bitarray_test_bit(g_bitarray_marcos, i) == false ) {
			bitarray_set_bit(g_bitarray_marcos,i);
			return i;
		}
		sem_post(&g_mutexgBitarray_marcos);
	}
	return -1;
}

int buscarFrameLibreSwap(){
	for (int i = 0; i < maxPaginasEnSwap; i++) {
		sem_wait(&g_mutexgBitarray_swap);
		if( bitarray_test_bit(g_bitarray_swap, i) == false ) {
			bitarray_set_bit(g_bitarray_swap,i);
			return i;
		}
		sem_post(&g_mutexgBitarray_swap);
	}

	return -1;
}

int ClockModificado() {

	// Libera y devuelve el numero de frame liberado
	int indiceDeMarco = -1;
	t_pagina* aux = NULL;
	t_pagina* paginaVictima = NULL;

	if (punteroClock ==  g_cantidadFrames) punteroClock = 0;
		for (int j = punteroClock; j < g_cantidadFrames; j++) {
			punteroClock = j;

			sem_wait(&g_mutextablasDePaginas);
			t_paginaAdministrativa* paginaGlobal = buscarPaginaAdministrativaPorFrame(tablasDePaginas, j);
			sem_post(&g_mutextablasDePaginas);

			aux = buscarFrameEnTablasDePaginas(paginaGlobal);
			if ( aux->flagPresencia == true && aux->flagModificado == true) {
				 aux->flagModificado = false;
			}
			else if (aux->flagPresencia == true && aux->flagModificado == false ) {
				 aux->flagPresencia = false;
			}
			else{

				paginaVictima = aux;
				cargarFrameASwap(paginaGlobal->nroFrame, paginaGlobal);
			}
		}
	// Libero el frame, destruyo pagina y devuelvo indice
	if( paginaVictima != NULL ){

		indiceDeMarco = paginaVictima->nroFrame;

		bitarray_clean_bit( g_bitarray_marcos, indiceDeMarco);

		return indiceDeMarco;
	}
	else return ClockModificado();
}

int framesNecesariosPorCantidadMemoria(int cantidadBytes){
	bool sinResto = (cantidadBytes %  g_configuracion->tamanioPagina) == 0 ;
	int aux = cantidadBytes / g_configuracion->tamanioPagina;
	return sinResto ? aux : aux+1;
}

int bytesNecesariosUltimoFrame(int cantidadBytes){
	bool sinResto = (cantidadBytes %  g_configuracion->tamanioPagina) == 0 ;
	int frames = framesNecesariosPorCantidadMemoria(cantidadBytes);
	return sinResto ? 0 : cantidadBytes - (frames * g_configuracion->tamanioPagina);
}

void destruirPrograma( t_programa* programa ){
	//free( segmento->nombreTabla ); free resto de campos?
	destruirSegmentosPrograma( programa->segmentos_programa);
	free( programa );
}

void destruirSegmentosPrograma( t_segmentos_programa* segmentos ){
	//free( segmento->nombreTabla ); free resto de campos?
	list_destroy_and_destroy_elements( segmentos->lista_segmentos, (void*) destruirSegmento );
	free( segmentos );
}

void destruirSegmento( t_segmento* segmento ){
	//free( segmento->nombreTabla ); free resto de campos?
	list_destroy_and_destroy_elements( segmento->tablaPaginas, (void*) destruirPagina );
	list_destroy_and_destroy_elements( segmento->heapsSegmento, (void*) destruirHeap );
	free( segmento );
}

void destruirSegmentoMap( t_segmento* segmento, bool borrarTodo ){
	if(borrarTodo)
	{
		list_destroy_and_destroy_elements( segmento->tablaPaginas, (void*) destruirPagina );
		free(segmento->mmap);
	}
	list_destroy_and_destroy_elements( segmento->heapsSegmento, (void*) destruirHeap );
	free( segmento );
}


void destruirPagina( t_pagina* pagina ){
	bitarray_clean_bit(g_bitarray_marcos,pagina->nroFrame);
	free( pagina );
}

void destruirHeap( t_heapSegmento* heap ){
	free( heap );
}

t_segmento* ultimoSegmentoPrograma(t_programa* programa){
	return list_get(programa->segmentos_programa->lista_segmentos,list_size(programa->segmentos_programa->lista_segmentos) -1);
}

int esDireccionLogicaValida(uint32_t direccionLogica, t_segmento* segmento){

	if(segmento->tipoSegmento == 2) return -1;

	int direccionLogicaAux = segmento->baseLogica + tamanio_heap ;
	bool encontrado = false;
	for (int i = 0; i < list_size(segmento->heapsSegmento) && !encontrado; i++)
	{
		t_heapSegmento* auxHeap = list_get(segmento->heapsSegmento,i);
		encontrado = (direccionLogicaAux  == direccionLogica);
		if(encontrado && 0  > auxHeap->t_size) return -1; //lo del 0 es para los heaps pisados que son invalidos
		else if(!encontrado &&  0  > auxHeap->t_size)  direccionLogicaAux += auxHeap->t_size * -1;
		else if(!encontrado) direccionLogicaAux += auxHeap->t_size + tamanio_heap;
		else return i;
	}

	if(!encontrado) return -1; //segmentation fault, TODO: buscar codigo syscall seg fault
}

void cambiarContenidoFrameMemoria(int nroFrame, void* nuevoContenido){
	t_contenidoFrame* contenidoCambiar = buscarContenidoFrameMemoria(nroFrame);
	contenidoCambiar->contenido = nuevoContenido;
}

void modificarContenidoPagina(t_pagina* pagina ,void* nuevoContenido, bool presencia){  //cambiar aca tambien contenido nuevo?
	pagina->flagPresencia = true;
	pagina->flagModificado = true;
	cambiarContenidoFrameMemoria(pagina->nroFrame, nuevoContenido);
}

void modificarPresencia(t_pagina* pagina , bool presencia, bool modifica){  //cambiar aca tambien contenido nuevo?
	pagina->flagPresencia = presencia;
	pagina->flagModificado = modifica;
	if(presencia) bitarray_set_bit( g_bitarray_marcos, pagina->nroFrame);
	else bitarray_clean_bit( g_bitarray_marcos, pagina->nroFrame );
}





