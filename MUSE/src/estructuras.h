#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>


typedef struct{
	int programaId;
	int socket;
	t_list* segmentos_programa;
}t_programa;

typedef struct{
	int programaId;
	int tamanioDireccionado;
	t_list* lista_segmentos;
}t_segmentos_programa;


typedef struct{
	int baseLogica;
	int tamanioDireccionado;
	t_list* tablaPaginas;
	pthread_t tid; //o id de programa
	int tipoSegmento;
}t_segmento;


typedef struct{
	uint32_t t_size;
	bool isFree;
}t_heapSegmento;

typedef struct{
	uint32_t t_size;
	bool isFree;
}t_mmapSegmento;

typedef struct{
	int indiceBitArray;
	uint32_t espacioLibre;
}t_sizeFreeFrame;

typedef struct{
	char* pathArchivo;
	t_list* tablaPaginas;
	pthread_t tid; //o id de programa
	sem_t semaforo;
}t_segmento_compartido;


typedef struct{
	int puertoConexion;
	int tamanioMemoria;
	int tamanioSwap;
	int tamanioPagina;
}t_configuracion;

typedef struct{
	int nroMarco;
	bool flagModificado;
	long flagPresencia;
}t_pagina;

typedef enum  {
	MUSE_ALLOC = 723,
	MUSE_FREE,
	MUSE_GET,
	MUSE_COPY,
	MUSE_MAP,
	MUSE_SYNC,
	MUSE_UNMAP,
}t_cod_operaciones_MUSE;


#endif
