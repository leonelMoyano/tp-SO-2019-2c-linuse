#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento * segmentoElegido;

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam, 1 );
	}
	else
	{
		int cantSegmentos = list_size(programa->segmentos_programa->lista_segmentos);
		segmentoElegido =  list_get(programa->segmentos_programa->lista_segmentos, cantSegmentos -1);
	}

	t_heapSegmento* metadata = malloc(sizeof(t_heapSegmento));
	uint32_t direccion = malloc(tam);
	metadata->isFree = false;
	metadata->t_size = tam;

	t_list* listaHeaps = segmentoElegido->heapsMetadata;

	list_add(listaHeaps,metadata);

	return direccion;

}
void procesarFree(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento* segmento = buscarDireccionEnPrograma(dir,programa->segmentos_programa);

	//int cantPaginasALiberar = framesNecesariosPorCantidadMemoria();

	int nroPagina = nroPaginaSegmento(dir, segmento->baseLogica);
	int offsetPagina = desplazamientoPaginaSegmento(dir, segmento->baseLogica);

	//Buscar heap metadata a liberar

	free(dir);

}

int procesarGet(void* dst, uint32_t src, size_t n, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento* segmento = buscarDireccionEnPrograma(src,programa->segmentos_programa);

	bool segmentoUnico = segmento->limiteLogico > src + n;

	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(n);

}

int procesarCopy(uint32_t dst, void* src, int n, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento* segmento = buscarDireccionEnPrograma(dst,programa->segmentos_programa);

	bool esExtendible = esSegmentoExtendible(programa->segmentos_programa, segmento);

	if(dst + n > segmento->limiteLogico && !esExtendible){ return -1;}




}

uint32_t procesarMap(char *path, size_t length, int flags, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
}

int procesarUnMap(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
}

