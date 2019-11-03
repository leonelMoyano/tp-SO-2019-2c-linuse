#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>
#include <stdbool.h>

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
	int punteroReemplazo;
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
	char* pathArchivo;
	t_list* tablaPaginas;
	pthread_t tid; //o id de programa
	sem_t semaforo;
	int punteroReemplazo;
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
	long flagPresencia; //para facilitar clock modificado
	t_registro * registro;
}t_pagina;

#endif
