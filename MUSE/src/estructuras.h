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
	int puertoConexion;
	int tamanioMemoria;
	int tamanioSwap;
	int tamanioPagina;
}t_configuracion;


typedef enum  {
	MUSE_INIT = 723,
	MUSE_ALLOC,
	MUSE_FREE,
	MUSE_GET,
	MUSE_COPY,
	MUSE_MAP,
	MUSE_SYNC,
	MUSE_UNMAP,
}t_cod_operaciones_MUSE;


typedef struct{
	int baseLogica;
	int limiteLogico;
	t_list* lista_segmentos;
	int nroUltimaPagina;
}t_segmentos_programa;


typedef struct{
	int programaId;
	int socket;
	t_segmentos_programa* segmentos_programa;
	int memoriaPerdida;
	int memoriaLiberada;
	int memoryLeaks;
	pthread_t tid;
}t_programa;

typedef struct{
	int baseLogica;
	int limiteLogico;
	t_list* tablaPaginas;
	int tipoSegmento;
	t_list* heapsSegmento;
}t_segmento;


typedef struct{
	uint32_t t_size;
	bool isFree;
}t_heapSegmento;


typedef struct{
	int nroFrame;
	int nroPagina;
	bool flagUso;
	bool flagModificado;
	bool flagPresencia;
	void* bytes;
}t_pagina;


//Tipos auxiliares

typedef struct{
	int nroFrame;
	uint32_t direccionLogica;
	t_heapSegmento* heap;
}t_heapDireccion;

typedef struct{
	int nroFrameSwap;
	int nroPagina;
	int socketPrograma;
	// identificar segmento por nro pagina global?
}t_paginaSwap;

typedef struct{
	int nroFrame;
	uint32_t espacioLibre;
	t_list* heapsFrame;
}t_heapFrameMetadata;

typedef struct{
	char* pathArchivo;
	t_list* tablaPaginas;
	pthread_t tid; //o id de programa
	sem_t semaforo;
}t_segmento_compartido;

#endif
