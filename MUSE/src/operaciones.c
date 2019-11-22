#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){
	t_programa * programa = buscarPrograma(socket);
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
			t_segmento * ultimoSegmento =  ultimoSegmentoPrograma(programa);
			if(ultimoSegmento->tipoSegmento == 2)//segmento mmap
			{
				segmentoElegido = crearSegmento(programa->segmentos_programa->baseLogica, tam, 1 );
			}
			else segmentoElegido = ultimoSegmento;

			direccionLogica = allocarEnPaginasNuevas(segmentoElegido,tam);
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



	//Buscar heap metadata a liberar

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

	//crear segmento
	// reservar paginas para satisfacer el length
}

int procesarSync(uint32_t addr, size_t len, int socket){
	t_programa * programa= buscarPrograma(socket);
}

uint32_t procesarUnMap(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(socket);

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

void RegistrarMetricasPrograma(t_programa* programa){

	int porcentaje = PorcentajeAsignacionMemoria(programa);
	int bytesLibres = EspacioLibre(ultimoSegmentoPrograma(programa));

	log_info( g_logger, "Programa:" );
	log_info( g_logger, "Porcentaje de asignación de memoria:",porcentaje );
	log_info( g_logger, "Espacio disponible último segmento:",bytesLibres );
	log_info( g_logger, "Memoria perdida:", programa->memoriaPerdida  );
	log_info( g_logger, "Memoria Liberada:", programa->memoriaLiberada  );
	log_info( g_logger, "Memory Leaks", programa->memoryLeaks );


}

void ActualizarLogMetricas(){

	list_iterate(programas, RegistrarMetricasPrograma);
	int cantidadBytes = SistemaMemoriaDisponible();
	log_info( g_logger, "La cantidad de memoria del sistema es de n bytes" );

}

int EspacioLibre(t_segmento* segmento){}
int PorcentajeAsignacionMemoria(t_programa* programa){}
int SistemaMemoriaDisponible(){}

