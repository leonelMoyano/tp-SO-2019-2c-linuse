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


t_list* crearTablaSegmentos() {
	t_list* aux = list_create();
	return aux;
}


t_segmento* crearSegmento(int direccionBase, int tamanio, int tipoSegmento ){
	t_segmento* segmentoNuevo = malloc( sizeof( t_segmento ) );
	//segmentoNuevo->nombreTabla = strdup( nombreDeTabla );
	segmentoNuevo->tablaPaginas = crearTablaPaginas();
	return segmentoNuevo;
}


t_pagina* crearPagina( t_registro* registro, int numeroDeMarco ){
	t_pagina* pagina = malloc( sizeof( t_pagina ) );
	pagina->flagPresencia  = true;
	pagina->flagModificado  = true;
	pagina->nroMarco        = numeroDeMarco;
	return pagina;
}

t_registro* crearRegistroYAgregarEnSegmento( int cantidadDeBytes, int programaId ){
	// Recorro tabla de marcos buscando marco vacio
	t_programa* programa = buscarPrograma(programas,programaId);

	int numeroDeMarco = buscarMarcoConEspacioLibre();
	if( numeroDeMarco == -1 ){
		log_debug( g_loggerDebug, "Todos los marcos ocupados hago Clock modificado" );
		numeroDeMarco = ClockModificado( programa->segmentos_programa );
		if( numeroDeMarco == -1 ){
			log_debug( g_loggerDebug, "Todos las paginas modificadas, seg fault?" );
		}
	}
	t_registro* registro;
	//agregarRegistroEnSegmento( segmento, registro, numeroDeMarco );
	return registro;
}


void agregarTablaSegmento(t_list * lista, t_segmento* tabla) {
	list_add(lista, tabla);
}


void agregarRegistroEnSegmento(t_segmento * segmento, t_registro* registro, int numeroDeMarco) {
	t_pagina * paginaNuevo = crearPagina( registro, numeroDeMarco );
	bitarray_set_bit( g_bitarray_marcos, numeroDeMarco );
	list_add( segmento->tablaPaginas, paginaNuevo );
}

t_segmento* buscarSegmento(t_list* segmentos,int direccionVirtual) {

	bool existeDireccionSegmento(void* segmento){
		t_segmento* segmentoBuscar = (t_segmento*) segmento;

		if (direccionVirtual != NULL) return segmentoBuscar->baseLogica < direccionVirtual   && direccionVirtual  < (  (segmentoBuscar->baseLogica + segmentoBuscar->tamanioDireccionado) );
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_segmento* segmentoBuscado = list_find(segmentos,existeDireccionSegmento);
	//sem_post(&g_mutex_tablas);
	return segmentoBuscado;
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



t_programa* buscarPrograma(t_list* programas, int Id) {

	bool existeIdPrograma(void* programa){
		t_programa* programaBuscar = (t_programa*) programa;

		if (Id != NULL) return programaBuscar->programaId == Id;
		return false;

	}

	//sem_wait(&g_mutex_tablas);
	t_programa* programaBuscado = list_find(programas,existeIdPrograma);
	//sem_post(&g_mutex_tablas);
	return programaBuscado;
}


t_segmento* buscarDireccionEnPrograma(int direccionVirtual, int programaId) {
	t_programa * programa = buscarPrograma( programas , programaId);
	t_segmento * segmento = buscarSegmento(programa->segmentos_programa,  direccionVirtual);

	/*if ( segmento == NULL ) {
		segmento = crearSegmento( nombre );
		agregarTablaSegmento( g_tabla_segmentos, segmento );
	}*/
	return segmento;
}

int nroPaginaSegmento(int direccionVirtual, int baseLogica){
	return (direccionVirtual - baseLogica) / g_configuracion->tamanioPagina ;
}

int desplazamientoPaginaSegmento(int direccionVirtual, int baseLogica){
	int nroPagina = nroPaginaSegmento(direccionVirtual,baseLogica);
	return (direccionVirtual - baseLogica) - (nroPagina * g_configuracion->tamanioPagina);
}

int buscarMarcoConEspacioLibre(int cantidadBytesNecesarios){
		int i = 0;
		if( bitarray_test_bit(g_bitarray_marcos, i) == false ) {
			return i;
		}

	return -1;
}


int ClockModificado(t_segmento* segmento) {

	//Manejar un indice global por segmento para saber donde quedo el ciclo
	// Libera y devuelve el numero de marco liberado
	int indiceDeMarco = -1;
	t_pagina* aux = NULL;
	t_pagina* paginaVictima = NULL;

	t_list* tablaDePaginas = segmento->tablaPaginas;
	if (punteroClock ==  list_size(tablaDePaginas)) punteroClock = 0;

		for (int j = punteroClock; j < list_size(tablaDePaginas); j++) {
			punteroClock = j;
			aux = list_get(tablaDePaginas, j);
			if ( aux->flagPresencia == true && aux->flagModificado == true) {
				 aux->flagModificado = false;
			}
			else if (aux->flagPresencia == true && aux->flagModificado == false ) {
				 aux->flagPresencia = false;
			}
			else{
				paginaVictima = aux;
				//if( aux->flagModificado == false) Escribir en disco
			}
		}
	// Libero el marco, destruyo pagina y devuelvo indice
	if( paginaVictima != NULL ){
		indiceDeMarco = paginaVictima->nroMarco;
		//Enviar victima a disco de swap
		bitarray_clean_bit( g_bitarray_marcos, indiceDeMarco );
		destruirPagina( paginaVictima );
		return indiceDeMarco;
	}
	else return ClockModificado(segmento);
}

int framesNecesariosPorCantidadMemoria(int cantidadBytes){

	return cantidadBytes / g_configuracion->tamanioPagina;
}

int bytesNecesariosUltimoFrame(int cantidadBytes){

	int framesCompletos = framesNecesariosPorCantidadMemoria(cantidadBytes) - 1;
	return cantidadBytes - (framesCompletos * g_configuracion->tamanioPagina);
}

void agregarFrameLibre(int bytesConsumidos, int nroFrame){
	t_sizeFreeFrame* frameLibre = malloc( sizeof( t_sizeFreeFrame ) );
	frameLibre->indiceBitArray = nroFrame;
	frameLibre->espacioLibre =  g_configuracion->tamanioPagina - bytesConsumidos - sizeof(t_sizeFreeFrame);
}

int verificarEspacioLibreUltimaPagina(int indiceFrame){
	t_sizeFreeFrame* frame = buscarFramePorIndice(framesLibres,indiceFrame);

	if(frame == NULL) return 0;
	return frame->espacioLibre;
}

void destruirSegmento( t_segmento* segmento ){
	//free( segmento->nombreTabla );
	list_destroy_and_destroy_elements( segmento->tablaPaginas, (void*) destruirPagina );
	free( segmento );
}


void destruirPagina( t_pagina* pagina ){
	//destruirRegistro( pagina->registro );
	free( pagina );
}
