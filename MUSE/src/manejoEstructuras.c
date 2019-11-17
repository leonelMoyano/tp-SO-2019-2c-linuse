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


t_segmento* crearSegmento(int direccionBase, int tamanio, int tipoSegmento ){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	segmentoNuevo->tablaPaginas = crearTablaPaginas();
	segmentoNuevo->heapsMetadata = crearListaHeapsMetadata();
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


t_pagina* crearPagina(int nroFrame ){
	t_pagina* pagina = malloc( sizeof( t_pagina ) );
	pagina->flagPresencia  = true;
	pagina->flagModificado  = true;
	pagina->nroFrame        = nroFrame;
	return pagina;
}

void registrarYAgregarEnSegmento( int cantidadDeBytes, t_programa* programa, t_segmento* segmentoElegido ){
	// Recorro tabla de marcos buscando marco vacio

	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(cantidadDeBytes);

		for(int i=0 ; cantPaginasAObtener > i;i++){
			int indice = buscarFrameLibre();
			agregarPaginaEnSegmento(segmentoElegido,indice);
		}

	int numeroDeMarco = buscarMarcoConEspacioLibre();
	if( numeroDeMarco == -1 ){
		log_debug( g_loggerDebug, "Todos los marcos ocupados hago Clock modificado" );
		numeroDeMarco = ClockModificado( programa->segmentos_programa );
		if( numeroDeMarco == -1 ){
			log_debug( g_loggerDebug, "Todos las paginas modificadas, seg fault?" );
		}
	}
	//guardarbytesEnMemoria( segmento, unosBYtes, numeroDeMarco );
}


void agregarTablaSegmento(t_list * lista, t_segmento* tabla) {
	list_add(lista, tabla);
}


void agregarPaginaEnSegmento(t_segmento * segmento, int numeroDeMarco) {
	t_pagina * paginaNuevo = crearPagina( numeroDeMarco );
	bitarray_set_bit( g_bitarray_marcos, numeroDeMarco );
	list_add( segmento->tablaPaginas, paginaNuevo );
}

t_segmento* buscarSegmento(t_list* segmentos,int direccionVirtual) {

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

t_pagina* buscarFrameEnTablasDePaginas(t_list* tablasPaginas, int nroFrame) {


	bool existeFrame(void* frame){
		t_pagina* frameBuscar = (t_pagina*) frame;

		if (nroFrame != NULL) return frameBuscar->nroFrame == nroFrame;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_pagina* frameBuscado = list_find(tablasPaginas,existeFrame);
	//sem_post(&g_mutex_tablas);
	return frameBuscado;
}

t_sizeFreeFrame* buscarFramePorIndice(t_list* frames, int indice) {

	bool existeFrame(void* frame){
		t_sizeFreeFrame* frameBuscar = (t_sizeFreeFrame*) frame;

		if (indice != NULL) return frameBuscar->indiceBitArray == indice;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_sizeFreeFrame* frameBuscado = list_find(frames,existeFrame);
	//sem_post(&g_mutex_tablas);
	return frameBuscado;
}



t_programa* buscarPrograma(t_list* programas, int socket) {

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


t_segmento* buscarDireccionEnPrograma(int direccionVirtual, t_segmentos_programa* segmentoPrograma) {
	t_segmento * segmento = buscarSegmento(segmentoPrograma,  direccionVirtual);
    //Convendria agregar nro de segmento??

	/*if ( segmento == NULL ) {
		segmento = crearSegmento( nombre );
		agregarTablaSegmento( g_tabla_segmentos, segmento );
	}*/
	return segmento;
}

bool esSegmentoExtendible(t_segmentos_programa* segmentos, t_segmento* segmento){

	return segmentos->limiteLogico > segmento->limiteLogico;
}

int nroPaginaSegmento(int direccionVirtual, int baseLogica){
	return (direccionVirtual - baseLogica) / g_configuracion->tamanioPagina ;
}

int desplazamientoPaginaSegmento(int direccionVirtual, int baseLogica){
	int nroPagina = nroPaginaSegmento(direccionVirtual,baseLogica);
	return (direccionVirtual - baseLogica) - (nroPagina * g_configuracion->tamanioPagina);
}

int buscarFrameLibre(){
		int i = 0;
		if( bitarray_test_bit(g_bitarray_marcos, i) == false ) {
			return i;
		}

	return -1;
}

int buscarMarcoConEspacioLibre(int cantidadBytesNecesarios){
		int i = 0;
		if( bitarray_test_bit(g_bitarray_marcos, i) == false ) {
			return i;
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
			aux = buscarFrameEnTablasDePaginas(tablasDePaginas,j);
			if ( aux->flagPresencia == true && aux->flagModificado == true) {
				 aux->flagModificado = false;
			}
			else if (aux->flagPresencia == true && aux->flagModificado == false ) {
				 aux->flagPresencia = false;
			}
			else{
				paginaVictima = aux;
				//if( aux->flagModificado == true) Escribir en disco
			}
		}
	// Libero el frame, destruyo pagina y devuelvo indice
	if( paginaVictima != NULL ){
		indiceDeMarco = paginaVictima->nroFrame;
		//Enviar victima a disco de swap
		bitarray_clean_bit( g_bitarray_marcos, indiceDeMarco );
		destruirPagina( paginaVictima );
		return indiceDeMarco;
	}
	else return ClockModificado();
}

int framesNecesariosPorCantidadMemoria(int cantidadBytes){

	return cantidadBytes / g_configuracion->tamanioPagina;
}

int bytesNecesariosUltimoFrame(int cantidadBytes){

	int framesCompletos = framesNecesariosPorCantidadMemoria(cantidadBytes) - 1;
	return cantidadBytes - (framesCompletos * g_configuracion->tamanioPagina);
}

int verificarEspacioLibreUltimaPagina(int indiceFrame){
	t_sizeFreeFrame* frame = buscarFramePorIndice(framesLibres,indiceFrame);

	if(frame == NULL) return 0;
	return frame->espacioLibre;
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
	//destruirRegistro( pagina->registro );
	free( pagina );
}

void leerDiscoSwap(int nroPagina){

	//1 byte igual 1 caracter

	int posicionBuscada = nroPagina * g_configuracion->tamanioPagina;

	// Leo hasta que encuentra un salto de linea o fin de string
		int i;
		for (i = 0; disco_swap[i] != '\n' && disco_swap[i] != '\0'; ++i){

		}
}




