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


typedef struct{
	char* puertoConexion;
	char* ipFileSystem;
	char* puertoFileSystem;
	char** ipSeeds;
	char** puertosSeeds;
	int tamanioMemoria;
	int retardoMemoria;
	int retardoFileSystem;
	int retardoJournal;
	int retardoGossiping;
	int numeroMemoria;
	int logDebug;
}t_configuraciones;


t_log* g_logger;
t_log* g_loggerDebug;
t_configuraciones* g_configuracion;
t_list* g_diccionarioConexiones; //de programas
t_list* g_tabla_segmentos;
t_bitarray * g_bitarray_marcos; // Para saber que marcos estan ocupados
void * g_granMalloc;

/*------------------------Funciones-------------------------*/
void			reservarEspacioMemoriaPrincipal	();
void            destruirGlobales                ();
void 			armarConfigMemoria				();



#endif

