/*
 * Contenido de header
 *     id: str ( nombre )
 *     version: numero
 *     bloque inicio de bitmap: numero
 *     tamanio de bitmap ( en bloques ): numero
 *     relleno: 0s
 */
#define TAMANIO_MAXIMO_NOMBRE 71

struct t_headerFS{
	char* nombre;
	int version;
	int inicioBitmap;
	int tamanioBitmap;
};

struct t_nodoFS{
	char* estado;
	char nombre[TAMANIO_MAXIMO_NOMBRE];
};
