#include "manejoEstructuras.h"


t_list* crearDiccionarioConexiones() {
	t_list* aux = list_create();
    //TODO: conexiones de los programas?
	return aux;
}

t_list* crearTablaPaginas() {
	t_list* aux = list_create();
	list_add(tablasDePaginas,aux);  //TODO: ver si reemplazar por TLB
	return aux;
}

t_list* crearListaHeapsMetadata() {
	t_list* aux = list_create();
	return aux;
}

t_segmentos_programa* crearSegmentosPrograma() {
	t_segmentos_programa* aux = malloc(sizeof(t_segmentos_programa));
	aux->baseLogica = direccionamientoLogicoActual;
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

t_segmento* crearSegmento(int direccionBase, int tamanio, int tipoSegmento, bool tablaDePaginasCompartida ){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	if(!tablaDePaginasCompartida) segmentoNuevo->tablaPaginas = crearTablaPaginas();
	segmentoNuevo->baseLogica = direccionBase;
	segmentoNuevo->limiteLogico = tamanio;
	segmentoNuevo->idSegmento = idSegmento;
	idSegmento++;
	return segmentoNuevo;
}

t_programa* crearPrograma(int socket){
	t_programa* programaNuevo = malloc( sizeof( t_programa ) );
	nroPrograma++;
	programaNuevo->programaId = nroPrograma;
	programaNuevo->segmentos_programa = crearSegmentosPrograma();
	programaNuevo->socket = socket;
	return programaNuevo;
}


t_pagina* crearPagina(int nroFrame, int nroPagina){
	t_pagina* pagina = malloc( sizeof( t_pagina ) );
	pagina->flagPresencia  = true; //si es mmap deberia ir en false
	pagina->flagModificado = false;
	pagina->nroFrame  = nroFrame;
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


void agregarTablaSegmento(t_list * lista, t_segmento* tabla) {
	list_add(lista, tabla);
}

void agregarContenido(int nroFrame, void* contenido){
	t_contenidoFrame* contenidoFrame = malloc( sizeof( t_contenidoFrame ) );
	contenidoFrame->nroFrame = nroFrame;
	contenidoFrame->contenido = contenido;
	list_add(contenidoFrames, contenidoFrame);
}

void agregarMapCompartido(char* path, int socketPrograma, int idSegmento, t_list* tablaPaginas){
	t_mapAbierto* map = malloc(sizeof(t_mapAbierto));
	map->socketPrograma = socketPrograma;
	map->path = path;
	map->idSegmento = idSegmento;
	map->tablaPaginas = tablaPaginas;
	list_add(mapeosAbiertosCompartidos, map);
}


void agregarPaginaEnSegmento(int socket, t_segmento * segmento, int numeroDeMarco) {
	t_pagina * paginaNuevo = crearPagina( numeroDeMarco, list_size( segmento->tablaPaginas) );
	bitarray_set_bit( g_bitarray_marcos, numeroDeMarco );
	list_add( segmento->tablaPaginas, paginaNuevo );
	list_add(tablasDePaginas, crearPaginaAdministrativa(socket,segmento->idSegmento,list_size(segmento->tablaPaginas),numeroDeMarco));
}

t_segmento* buscarSegmento(t_list* segmentos,uint32_t direccionVirtual) {

	bool existeDireccionSegmento(void* segmento){
		t_segmento* segmentoBuscar = (t_segmento*) segmento;

		if (direccionVirtual != NULL) return segmentoBuscar->baseLogica < direccionVirtual   && direccionVirtual  < (  (segmentoBuscar->baseLogica + segmentoBuscar->limiteLogico) );
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_segmento* segmentoBuscado = list_find(segmentos,existeDireccionSegmento);
	//sem_post(&g_mutex_tablas);
	return segmentoBuscado;
}

t_mapAbierto* buscarMapeoAbierto(char* path) {

	bool existePath(void* mapAbierto){
		t_mapAbierto* mapAbiertoBuscar = (t_mapAbierto*) mapAbierto;

		if (path != NULL) return string_equals_ignore_case(mapAbiertoBuscar->path, path);
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_mapAbierto* mapAbiertoBuscado = list_find(mapeosAbiertosCompartidos,existePath);
	//sem_post(&g_mutex_tablas);
	return mapAbiertoBuscado;
}



t_segmento* buscarSegmentoId(t_list* segmentos,int idSemgneto) {

	bool existeDireccionSegmento(void* segmento){
		t_segmento* segmentoBuscar = (t_segmento*) segmento;

		if (idSemgneto != NULL) return segmentoBuscar->idSegmento == idSemgneto;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_segmento* segmentoBuscado = list_find(segmentos,existeDireccionSegmento);
	//sem_post(&g_mutex_tablas);
	return segmentoBuscado;
}


t_contenidoFrame* buscarContenidoFrameMemoria(int nroFrame) {

	bool existeContenidoFrame(void* contenidoFrame){
		t_contenidoFrame* contenidoBuscar = (t_contenidoFrame*) contenidoFrame;

		if(nroFrame != NULL) return contenidoBuscar->nroFrame == nroFrame;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_contenidoFrame* contenidoBuscado = list_find(contenidoFrames,existeContenidoFrame);
	//sem_post(&g_mutex_tablas);
	return contenidoBuscado;
}

int traerFrameDePaginaEnSwap(int socketPrograma,int idSegmento, int nroPagina) {

	bool existeFrame(void* frame){
		t_paginaAdministrativa* paginaBuscar = (t_paginaAdministrativa*) frame;
		if (nroPagina != NULL) return paginaBuscar->nroPagina == nroPagina && paginaBuscar->socketPrograma == socketPrograma && paginaBuscar->idSegmento == idSegmento;
		return false;
	}

	//sem_wait(&g_mutex_tablas);
	t_paginaAdministrativa* paginaBuscada = list_find(paginasEnSwap,existeFrame);
	//sem_post(&g_mutex_tablas);
	return paginaBuscada->nroFrame;
}

t_paginaAdministrativa* buscarPaginaAdministrativa(t_list* SwapOPrincipal, int nroFrame){

	bool existeFrame(void* frame){
		t_paginaAdministrativa* frameBuscar = (t_paginaAdministrativa*) frame;

		if (nroFrame != NULL) return frameBuscar->nroFrame == nroFrame;
		return false;
	}
	//sem_wait(&g_mutex_tablas);
	t_paginaAdministrativa* paginaAdministrativa = list_find(SwapOPrincipal,existeFrame);
	//sem_post(&g_mutex_tablas);

	return paginaAdministrativa;

}

t_pagina* buscarFrameEnTablasDePaginas(t_paginaAdministrativa* paginaABuscar) {

	t_programa * programa= buscarPrograma(paginaABuscar->socketPrograma);
	t_segmento* segmento = buscarSegmento(programa->segmentos_programa,paginaABuscar->idSegmento);
	t_pagina* paginaBuscada = list_get(segmento->tablaPaginas,paginaABuscar->nroPagina);
	return paginaBuscada;
}


t_programa* buscarPrograma(int socket) {

	bool existeIdPrograma(void* programa){
		t_programa* programaBuscar = (t_programa*) programa;

		if (socket != NULL) return programaBuscar->socket == socket;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_programa* programaBuscado = list_find(programas,existeIdPrograma);
	//sem_post(&g_mutex_tablas);
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
		if( bitarray_test_bit(g_bitarray_marcos, i) == false ) {
			bitarray_set_bit(g_bitarray_marcos,i);
			return i;
		}
	}
	return -1;
}

int buscarFrameLibreSwap(){
	for (int i = 0; i < maxPaginasEnSwap; i++) {
		if( bitarray_test_bit(g_bitarray_swap, i) == false ) {
			bitarray_set_bit(g_bitarray_swap,i);
			return i;
		}
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
			t_paginaAdministrativa* paginaGloblal = buscarPaginaAdministrativa(tablasDePaginas, j);
			aux = buscarFrameEnTablasDePaginas(paginaGloblal);
			if ( aux->flagPresencia == true && aux->flagModificado == true) {
				 aux->flagModificado = false;
			}
			else if (aux->flagPresencia == true && aux->flagModificado == false ) {
				 aux->flagPresencia = false;
			}
			else{
				paginaVictima = aux;
				//TODO: buscar contenido de void* a estructuca de contenidoframe
				void* unosBytes;
				if( aux->flagModificado == true) cargarPaginaEnSwap(unosBytes,paginaGloblal->nroPagina,paginaGloblal->socketPrograma,paginaGloblal->idSegmento);
			}
		}
	// Libero el frame, destruyo pagina y devuelvo indice
	if( paginaVictima != NULL ){
		indiceDeMarco = paginaVictima->nroFrame;

		//aca me parece que deberua modificarla nada mas la pagina
		bitarray_clean_bit( g_bitarray_marcos, indiceDeMarco);
		destruirPagina( paginaVictima );

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

	int framesCompletos = framesNecesariosPorCantidadMemoria(cantidadBytes) - 1;
	return cantidadBytes - (framesCompletos * g_configuracion->tamanioPagina);
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
	free( segmento );
}


void destruirPagina( t_pagina* pagina ){
	bitarray_clean_bit(g_bitarray_marcos,pagina->nroFrame);
	free( pagina );
}


t_segmento* ultimoSegmentoPrograma(t_programa* programa){
	return list_get(programa->segmentos_programa->lista_segmentos,list_size(programa->segmentos_programa->lista_segmentos) -1);
}

t_heapDireccion* buscarHeapSegmento(uint32_t direccionABuscar, t_segmento* segmento){
	t_heapSegmento* auxHeap = NULL;
	uint32_t direccionHeap = 0;
	bool encontrado = false;

	for (int i = 0; i < list_size(segmento->heapsSegmento) && !encontrado; i++) {
		auxHeap = list_get(segmento->heapsSegmento,i);
		direccionHeap += tamanio_heap;
		if(direccionHeap == direccionABuscar) encontrado == true;
		else direccionHeap += auxHeap->t_size;
	}

	t_heapDireccion* heapBuscado = malloc(sizeof(t_heapDireccion));
	heapBuscado->heap = auxHeap;
	heapBuscado->direccionLogica = direccionHeap + tamanio_heap;
}




