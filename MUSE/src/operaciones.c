#include "operaciones.h"

uint32_t procesarAlloc(uint32_t tam, int socket){
	t_programa * programa= buscarPrograma(programas,socket);

	if(list_is_empty(programa->segmentos_programa->lista_segmentos))
	{
		t_segmento* nuevoSegmento = crearSegmento(programa->segmentos_programa->baseLogica, tam, 1 );
	}
	else
	{
		int cantSegmentos = list_size(programa->segmentos_programa->lista_segmentos);
		t_segmento * ultimoSegmento =  list_get(programa->segmentos_programa->lista_segmentos, cantSegmentos -1);
	}

}
void procesarFree(uint32_t dir, int socket){
	t_programa * programa= buscarPrograma(programas,socket);
	t_segmento* segmento = buscarDireccionEnPrograma(dir,programa->segmentos_programa);

	//int cantPaginasALiberar = framesNecesariosPorCantidadMemoria();

	int nroPagina = nroPaginaSegmento(dir, segmento->baseLogica);
	int offsetPagina = desplazamientoPaginaSegmento(dir, segmento->baseLogica);

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

	int cantPaginasAObtener = framesNecesariosPorCantidadMemoria(n);

	for(int i=0 ; cantPaginasAObtener > i;i++){
		int indice = buscarFrameLibre();
		//copiar parte de lo que tenga que copiar
	}


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

