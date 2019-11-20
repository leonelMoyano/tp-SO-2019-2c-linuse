#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento * segmentoElegido;
	uint32_t direccionLogica = 0;

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam, 1 );
		direccionLogica = allocarEnPaginasNuevas(segmentoElegido,tam);
	}
	else
	{
		direccionLogica = allocarEnHeapLibre(tam,programa->segmentos_programa);
		if(direccionLogica == 0) //Si es direccion = 0 no encontro y hay que extender el ultimo segmento
		{
			int cantSegmentos = list_size(programa->segmentos_programa->lista_segmentos);
			segmentoElegido =  list_get(programa->segmentos_programa->lista_segmentos, cantSegmentos -1);
			direccionLogica = allocarEnPaginasNuevas(segmentoElegido,tam);
		}
	}

	return direccionLogica;

}
void procesarFree(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento* segmento = buscarDireccionEnPrograma(dir,programa->segmentos_programa);

	//si es segmento mmap no debera liberar nada o error?


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

	//crear segmento
	// reservar paginas para satisfacer el length
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
}

uint32_t procesarUnMap(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(programas,socket);

}


uint32_t allocarEnHeapLibre(uint32_t cantidadBytesNecesarios, t_segmentos_programa* segmentos){
	t_segmento* segmentoBuscar = NULL;
	t_heapSegmento* auxHeap = NULL;
	t_heapSegmento* heapBuscado = NULL;
	int tamanio_heap = 5;
	uint32_t direccionHeap = 0;
	bool encontrado = false;
	int i = 0;

	for(int j = 0; j < list_size(segmentos->lista_segmentos) && !encontrado; j++)
	{
		segmentoBuscar = list_get(segmentos->lista_segmentos,j);
		direccionHeap = segmentos->baseLogica;
		if(segmentoBuscar->tipoSegmento)
		{ //es heap
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
		//agregar nuevo heap libre si sobra espacio, verificar si el anterior o posterior esta free , compactar
		direccionHeap += tamanio_heap;
	}


	return direccionHeap;
}

uint32_t allocarEnPaginasNuevas(t_segmento* segmentoAExtender, uint32_t cantidadBytesNecesarios ){

	uint32_t direccionHeap = 0;
	return direccionHeap;

}


