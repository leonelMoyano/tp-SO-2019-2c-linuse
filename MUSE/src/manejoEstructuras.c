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
	pagina->registro        = registro;
	pagina->nroMarco        = numeroDeMarco;
	return pagina;
}

t_registro* crearRegistroYAgregarEnSegmento( uint16_t key, char* valor, t_segmento * segmento ){
	// Recorro tabla de marcos buscando marco vacio
	int numeroDeMarco = buscarMarcoVacio();
	if( numeroDeMarco == -1 ){
		log_debug( g_loggerDebug, "Todos los marcos ocupados hago Clock modificado" );
		numeroDeMarco = ClockModificado( g_tabla_segmentos );
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
	t_segmento * segmento = buscarSegmento(direccionVirtual);

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

//TODO este no servira
t_pagina* buscarPaginaClave( t_list * tablaPaginas, uint16_t clave) {
	bool existeKey(void* pagina){
		t_pagina* paginaBuscar = (t_pagina*) pagina;

		return paginaBuscar->registro->clave == clave;
	}

	//sem_wait(&g_mutex_tablas);
	t_pagina* paginaBuscada = list_find(tablaPaginas,existeKey);
	//sem_post(&g_mutex_tablas);
	return paginaBuscada;
}

int buscarMarcoVacio(){

		//if( bitarray_test_bit(g_bitarray_marcos, i) == false ) return i;

	return -1;
}


int ClockModificado(t_segmento* segmento) {

	//Manejar un indice global por segmento para saber donde quedo el ciclo
	// Libera y devuelve el numero de marco liberado
	int indiceDeMarco = -1;
	t_pagina* aux = NULL;
	t_pagina* paginaVictima = NULL;

	t_list* tablaDePaginas = segmento->tablaPaginas;
		for (int j = segmento->punteroReemplazo; j < list_size(tablaDePaginas); j++) {
			aux = list_get(tablaDePaginas, j);
			if ( aux->flagPresencia == true) {
				 aux->flagPresencia = false;
			}
			else if (aux->flagModificado == true ) {
				 aux->flagModificado = false;
			}
			else{
				paginaVictima = aux;
				//actualizo puntero donde quedo el clock en estructura
				segmento->punteroReemplazo = j;
			}
		}
	// Libero el marco, destruyo pagina y devuelvo indice
	if( paginaVictima != NULL ){
		indiceDeMarco = paginaVictima->nroMarco;
		bitarray_clean_bit( g_bitarray_marcos, indiceDeMarco );
		destruirPagina( paginaVictima );
	}
	return indiceDeMarco;
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
