#ifndef MANEJOESTRUCTURAS_H_
#define MANEJOESTRUCTURAS_H_

#include "MUSE.h"
#include <stdbool.h>
#include <commons/bitarray.h>

t_list*			crearDiccionarioConexiones		();
t_segmentos_programa* crearSegmentosPrograma	();
t_list*			crearListaHeapsMetadata			();
t_list*			crearTablaProgramas				();
t_list*			crearTablaPaginas				();
t_list*         crearTablaSegmentos             ();
t_segmento*     crearSegmento                   (int direccionBase, int tamanio, int tipoSegmento);
t_pagina*       crearPagina                     (int numeroDeMarco, int nroPagina);
t_paginaAdministrativa* crearPaginaAdministrativa(int socketPrograma, int idSegmento,int nroPagina, int nroFrame);
void 			agregarTablaSegmento			(t_list * lista, t_segmento* tabla);
void 			agregarPaginaEnSegmento		    (int socket, t_segmento * segmento, int numeroDeMarco);
void			registrarYAgregarEnSegmento		( int cantidadDeBytes, t_programa* programa, t_segmento* segmentoElegido );
t_segmento*		buscarSegmentoId				(t_list* segmentos,int idSemgneto);
t_segmento*	    buscarSegmento					(t_list* segmentos,uint32_t direccionVirtual);
t_segmento* 	ultimoSegmentoPrograma			(t_programa* programa);
t_programa* 	buscarPrograma				    (int socket);
int             buscarFrameLibre                ();
t_pagina*		buscarFrameEnTablasDePaginas	(t_paginaAdministrativa* paginaABuscar);
t_paginaAdministrativa* buscarPaginaAdministrativa(t_list* SwapOPrincipal, int nroFrame);
int				traerFrameDePaginaEnSwap		(int socketPrograma,int idSegmento, int nroPagina);
t_heapSegmento* buscarHeapConEspacioLibre		(int cantidadBytesNecesarios, t_segmento* segmento);
int             ClockModificado                 ();



int				bytesNecesariosUltimoFrame		(int cantidadBytes);
int				framesNecesariosPorCantidadMemoria(int cantidadBytes);
int				desplazamientoPaginaSegmento	(uint32_t direccionVirtual, int baseLogica);
int				nroPaginaSegmento				(uint32_t direccionVirtual, int baseLogica);
bool			esSegmentoExtendible			(t_segmentos_programa* segmentos, t_segmento* segmento);

/**** Destuir estructuras ****/
void       destruirSegmento             (t_segmento* segmento);
void       destruirPagina               (t_pagina* pagina);
void	   destruirPrograma				( t_programa* programa );
void 	   destruirSegmentosPrograma	( t_segmentos_programa* segmentos );

#endif /* MANEJOESTRUCTURAS_H_ */
