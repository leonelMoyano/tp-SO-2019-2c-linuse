#ifndef operaciones_h
#define operaciones_h

#include "estructuras.h"
#include "manejoEstructuras.h"


uint32_t		 procesarAlloc		(uint32_t tam, int socket);
void			 procesarFree		(uint32_t dir, int socket);
int				 procesarGet		(void* dst, uint32_t src, size_t n, int socket);
int				 procesarCopy		(uint32_t dst, void* src, int n, int socket);
uint32_t		 procesarMap		(char *path, size_t length, int flags, int socket);
int 			 procesarSync		(uint32_t addr, size_t len, int socket);
uint32_t		 procesarUnMap		(uint32_t dir, int socket);


uint32_t		 allocarEnHeapLibre(uint32_t cantidadBytesNecesarios, t_segmentos_programa* segmentos);
uint32_t		 allocarEnPaginasNuevas(int socket,t_segmento* segmentoAExtender, uint32_t cantidadBytesNecesarios );

void*			 leerFrameSwap		(int nroMarco);
void			 escribirFrameSwap	(int nroMarco, void* contenido);
void			 TraerPaginaDeSwap	(int socketPrograma, int nroPagina, int idSegmento);
void			 cargarPaginaEnSwap	(void* bytes,int nroPagina, int socketPrograma, int idSegmento);

void*			 leerArchivoCompartido();
void*			 escribirEnArchivoCompartido();

void			 ActualizarLogMetricas();
void 			 RegistrarMetricasPrograma(t_programa* programa);
uint32_t		 EspacioLibre(t_segmento* segmento);
int				 PorcentajeAsignacionMemoria(t_programa* programa);
int				 SistemaMemoriaDisponible();

#endif
