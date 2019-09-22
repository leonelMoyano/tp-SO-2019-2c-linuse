#include <commons/collections/list.h>
#include <stdbool.h>


typedef struct{
	char* nombreTabla;
	t_list* tablaPaginas;
}t_segmento;

typedef struct{
	int nroMarco;
	bool flagModificado;
	long flagPresencia; //para facilitar clock modificado
	t_registro * registro;
}t_pagina;
