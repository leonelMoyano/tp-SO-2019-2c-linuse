// Este es un util para crear un archivo donde va a vivir el FS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/bitarray.h>
#define BLOCK_SIZE 4096 // en bytes
#define CANTIDAD_BLOQUES_NODOS 1024 // cantidad de entradas posibles para la tabla de nodos (1 bloque = 1 entrada)
#define CANTIDAD_BLOQUES_HEADER 1   // cantidad de bloques reservados para el header
#define MENSAJE_ERROR "Malos argumentos\n"\
	                  "Modos posibles:\n"\
	                  "  Crear archivo de FS, sin flag (modo por defecto)\n"\
	                  "    path del archivo\n"\
					  "    cantidad de bloues\n"\
	                  "  Imprimir archivo en formato\n"\
	                  "    -f: flag para activar modo\n"\
	                  "    path del archivo\n"
#define FS_NOMBRE "SAC"
#define FS_VERSION 1

void rutinaCreacionArchivo(char*, int);
void subrutinaEscribirHeader(FILE*, int);
void subrutinaEscribirBitmap(FILE*, int);
void subrutinaEscribirNodos(FILE*, int);
void subrutinaEscribirDatos(FILE*, int);
void rutinaPrintArchivo(char*);
int cantidadBloquesBitmap(int);
int cantidadBloquesDatos(int);

int main(int argc, char *argv[]) {
	/*
	 * para debuggear que argumentos llegan
	printf("cantidad de args %d\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("este es el %d arg: %s\n", i, argv[i]);
	}
	*/
	if(argc != 3){
		perror(MENSAJE_ERROR);
		exit(-1);
	}

	if (strcmp(argv[1], "-f") == 0) {
		rutinaPrintArchivo(argv[2]);
	}
	rutinaCreacionArchivo(argv[1], atoi(argv[2]));
	return 0;
}

void rutinaCreacionArchivo(char* path, int cantidadBloques){
	// Creo el archivo
	FILE *file_pointer = fopen(path, "wb");

	subrutinaEscribirHeader(file_pointer, cantidadBloques);
	subrutinaEscribirBitmap(file_pointer, cantidadBloques);
	subrutinaEscribirNodos(file_pointer, cantidadBloques);
	subrutinaEscribirDatos(file_pointer, cantidadBloques);

	// Close the file
	fclose(file_pointer);
}

void subrutinaEscribirHeader(FILE* file_pointer, int cantidadBloques) {
	char* name = FS_NOMBRE;
	int version = FS_VERSION;
	int bloqueInicioBitmap = CANTIDAD_BLOQUES_HEADER; // Porque el bitmap es lo que sigue despues del header
	int cantBloquesBitmap = cantidadBloquesBitmap(cantidadBloques);
	int cantidadBytesRelleno = BLOCK_SIZE - strlen(name) - 1 - sizeof(int) * 3;

	/*
	 * Contenido de header
	 *     id: str ( nombre )
	 *     version: numero
	 *     bloque inicio de bitmap: numero
	 *     tamanio de bitmap ( en bloques ): numero
	 *     relleno: 0s
	 */
	char zero = 0;
	fwrite(name, strlen(name), 1, file_pointer);
	fwrite(&zero, sizeof(char), 1, file_pointer);
	fwrite(&version, sizeof(int), 1, file_pointer);
	fwrite(&bloqueInicioBitmap, sizeof(int), 1, file_pointer);
	fwrite(&cantBloquesBitmap, sizeof(int), 1, file_pointer);
	fwrite(&zero, sizeof(char), cantidadBytesRelleno, file_pointer);
}

void subrutinaEscribirBitmap(FILE* file_pointer, int cantidadBloques){
	int cantBloquesBitmap = cantidadBloquesBitmap(cantidadBloques);
	int bitmapSize = cantBloquesBitmap * BLOCK_SIZE;
	char* bitmapMem = malloc(bitmapSize);

	// inicializo el bitmap entero en 0
	memset(bitmapMem, 0, bitmapSize);
	t_bitarray* bitmap = bitarray_create_with_mode(bitmapMem, bitmapSize, MSB_FIRST);

	// seteo 1s en los bits del header y el bitmap
	int bloquesReservados = cantidadBloques - cantidadBloquesDatos(cantidadBloques);
	for(int i = 0; i < bloquesReservados; i++){
		bitarray_set_bit(bitmap, i);
	}

	// vuelco el bitmap al archivo
	fwrite(bitmapMem, bitmapSize, 1, file_pointer);

	bitarray_destroy(bitmap);
}

void subrutinaEscribirNodos(FILE* file_pointer, int cantidadBloques){
	// Por cada bloque de nodo
	char zero = 0;
	fwrite(&zero, sizeof(zero), CANTIDAD_BLOQUES_NODOS * BLOCK_SIZE, file_pointer);
}

void subrutinaEscribirDatos(FILE* file_pointer, int cantidadBloques){
	// Por cada bloque de datos
	char zero = 0;
	fwrite(&zero, sizeof(char), cantidadBloquesDatos(cantidadBloques) * BLOCK_SIZE, file_pointer);
}

void rutinaPrintArchivo(char* path){
	// TODO implementar
}

/*
 * Devuelve cantidad de bloques reservado para el bitmap
 */
int cantidadBloquesBitmap(int cantidadBloquesTotales){
	return cantidadBloquesTotales / 8;
}

/*
 * Devuelve cantidad de bloques reservado para datos
 */
int cantidadBloquesDatos(int cantidadBloquesTotales){
	return cantidadBloquesTotales - CANTIDAD_BLOQUES_NODOS - CANTIDAD_BLOQUES_HEADER - cantidadBloquesBitmap(cantidadBloquesTotales);
}
