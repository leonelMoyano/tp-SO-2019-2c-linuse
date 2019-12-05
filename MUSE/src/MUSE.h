#ifndef MUSE_H_
#define MUSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <biblioNOC/serializaciones.h>
#include <biblioNOC/conexiones.h>
#include <biblioNOC/estructurasAdministrativas.h>
#include <biblioNOC/paquetes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <signal.h>
#include "estructuras.h"
#include "operaciones.h"
#include "util.h"

#define RUTACONFIG "csdasd"
#define RUTASWAP "ASDAS"
#define LIBMUSE 400

int idSegmento;
int nroPrograma;
int punteroClock;
int direccionamientoLogicoActual;
void* archivoSwap;

t_log* g_logger;
t_log* g_loggerDebug;
int g_cantidadFrames;
t_configuracion * g_configuracion;
t_config* g_config;
t_list* g_diccionarioConexiones; //de programas
pthread_t hiloServidor;


t_bitarray * g_bitarray_marcos; // Para saber que marcos estan ocupados
t_list * contenidoFrames;
t_list* tablasDePaginas; // o TLB para hacer reemplazo global

t_bitarray * g_bitarray_swap;
FILE ** disco_swap;
t_list * paginasEnSwap;
int maxPaginasEnSwap;

void * g_granMalloc;

t_list* programas;
int tamanio_heap;

/*------------------------Funciones-------------------------*/
void 			attendConnection				(int socketCliente);
void			reservarEspacioMemoriaPrincipal	();
void            destruirGlobales                ();
void 			armarConfigMemoria				();
t_paquete* 		procesarPaqueteLibMuse			(t_paquete* paquete, int cliente_fd);

#endif

