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

	//Buscar heap metadata a liberar - buscarHeapSegmento

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

	mapearArchivoMUSE(path,length,flags);

	int tamanioSegmento = length; //calcular por cantidad de paginas necesarias

	t_segmento * nuevoSegmento = crearSegmento(programa->segmentos_programa->limiteLogico,length,1);
	list_add(programa->segmentos_programa,crearSegmento(programa->segmentos_programa->limiteLogico,length,1));
	programa->segmentos_programa->limiteLogico += tamanioSegmento;

	allocarEnPaginasNuevas(nuevoSegmento, length);

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
	uint32_t direccionHeap = 0;
	bool encontrado = false;
	int i = 0;

	for(int j = 0; j < list_size(segmentos->lista_segmentos) && !encontrado; j++)
	{
		segmentoBuscar = list_get(segmentos->lista_segmentos,j);
		direccionHeap = segmentos->baseLogica;
		if(segmentoBuscar->tipoSegmento)
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
		//agregar nuevo heap libre si sobra espacio, verificar si el anterior o posterior esta free , compactar
		direccionHeap += tamanio_heap;
	}


	return direccionHeap;
}

uint32_t allocarEnPaginasNuevas(t_segmento* segmentoAExtender, uint32_t cantidadBytesNecesarios ){

	int cantPaginasNecesarias = framesNecesariosPorCantidadMemoria(cantidadBytesNecesarios);

	int i;

	for (i = 0; cantPaginasNecesarias > i ; ++i){
		int indiceFrame = buscarFrameLibre();
		if(indiceFrame == -1) indiceFrame = ClockModificado();
		else
		agregarPaginaEnSegmento(segmentoAExtender,indiceFrame);
		bitarray_set_bit(g_bitarray_marcos,indiceFrame);
	}


	uint32_t direccionHeap = 0;
	return direccionHeap;

}

int freeDireccionLogicaValida(uint32_t direccionLogica, t_segmento* segmento){


	int direccionLogicaAux = 0;
	bool encontrado = false;
	for (int i = 0; i < list_size(segmento->heapsSegmento) && !encontrado; i++)
	{
		t_heapSegmento* auxHeap = list_get(segmento->heapsSegmento,i);
		encontrado = !auxHeap->isFree && (direccionLogicaAux + tamanio_heap == direccionLogica);
		if(!encontrado) direccionLogicaAux += auxHeap->t_size + tamanio_heap;
		else auxHeap->isFree = true; //fijar si tengo que compactar heaps
	}

	if(!encontrado) return -1; //segmentation fault
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

	int marcoEnSwap = traerFrameDePaginaEnSwap(socketPrograma,nroPagina);

}

char* leerFrameSwap(int nroMarco){

	*disco_swap = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer
	size_t tamArc = g_configuracion->tamanioSwap;

	// Leo el total del archivo y lo asigno al buffer
	void * dataArchivo = calloc( 1, tamArc + 1 );
	fread( dataArchivo, tamArc, 1, *RUTASWAP );

	//hacer malloc de tamanio de pagina, y luego enviar el void* , hacer lectura por bloques
	//tener en cuenta que lo que voy a mandar tiene el tamannio a alloquear mas el heapmetadata

	char ** listaPaginas = string_split(dataArchivo, "\n");

	char* asd = list_get(listaPaginas,nroMarco);

	list_replace(listaPaginas,nroMarco,'\0');

	bitarray_clean_bit(g_bitarray_swap,nroMarco);

	fclose(*disco_swap);
}

char* escribirFrameSwap(int nroMarco, void* contenido, size_t largo){

	*disco_swap = fopen(RUTASWAP, "r+");

	// Tamaño del archivo que voy a leer
	size_t tamArc = g_configuracion->tamanioSwap;

	// Leo el total del archivo y lo asigno al buffer
	void * dataArchivo = calloc( 1, tamArc + 1 );
	fread( dataArchivo, tamArc, 1, *RUTASWAP );

	int indiceArchivo = nroMarco * g_configuracion->tamanioPagina;
	fseek(disco_swap, indiceArchivo ,SEEK_SET);

	void* bloque = malloc(g_configuracion->tamanioPagina);
	fwrite(bloque,g_configuracion->tamanioPagina,1,disco_swap);

	bitarray_set_bit(g_bitarray_swap,nroMarco);

	fclose(*disco_swap);
}

void cargarPaginaEnSwap(void* bytes,int nroPagina, int nroPrograma, int idSegmento){

	int nroFrame = buscarFrameLibreSwap();


}
