#ifndef MUSE_H_
#define MUSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <biblioteca/serializaciones.h>
#include <biblioteca/conexiones.h>
#include <biblioteca/estructurasAdministrativas.h>
#include <biblioteca/paquetes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <signal.h>
#include "estructuras.h"

#define RUTACONFIG "csdasd"
#define RUTASWAP "ASDAS"

t_log* g_logger;
t_log* g_loggerDebug;
int g_cantidadRegistrosPosibles;
t_configuracion * g_configuracion;
t_config* g_config;
t_list* g_diccionarioConexiones; //de programas

t_list* g_tabla_segmentos;//la dejo por las dudas, pero se utilizaran una lista de segmentos por programa

t_bitarray * g_bitarray_marcos; // Para saber que marcos estan ocupados
t_list * listaHeaps; //esto no seria mas preciso??

t_bitarray * g_bitarray_swap; // DUDA ---probablemente, ver si manejar asi o con una lista
char * disco_swap;
t_list * paginasEnSwap;
int maxPaginasEnSwap;

void * g_granMalloc;

t_list* programas;

/*------------------------Funciones-------------------------*/
void			reservarEspacioMemoriaPrincipal	();
void            destruirGlobales                ();
void 			armarConfigMemoria				();

#endif

