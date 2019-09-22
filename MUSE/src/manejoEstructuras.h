#ifndef MANEJOESTRUCTURAS_H_
#define MANEJOESTRUCTURAS_H_

#include "MUSE.h"
#include <stdbool.h>
#include <commons/bitarray.h>

//TODO - definir estructura de memoria - usaremos registros fijos?

t_list* 		     crearDiccionarioConexiones		 ();
t_list*			     crearTablaPaginas				 ();
t_list*              crearTablaSegmentos             ();
t_segmento*          crearSegmento                   (char* nombreDeTabla);
t_pagina*            crearPagina                     (t_registro* registro, int numeroDeMarco);
t_registro*          crearRegistroYAgregarEnSegmento (uint16_t key, char* valor, t_segmento * segmento);
void 			     agregarTablaSegmento			 (t_list * lista, t_segmento* tabla);
void 			     agregarRegistroEnSegmento		 (t_segmento * segmento, t_registro * registro, int numeroDeMarco);
t_segmento*	         buscarSegmento					 (t_list* tablas, char* nombre);
t_segmento*          buscarOCrearSegmento            (t_list* tablas, char* nombre);
t_pagina*            buscarPaginaClave               (t_list * tablaPaginas, uint16_t clave);
int                  buscarMarcoVacio                ();
int                  ClockModificado                             (t_list* tablaDeSegmentos);

/**** Destuir estructuras ****/
void       destruirSegmento             (t_segmento* segmento);
void       destruirPagina               (t_pagina* pagina);

#endif /* MANEJOESTRUCTURAS_H_ */
