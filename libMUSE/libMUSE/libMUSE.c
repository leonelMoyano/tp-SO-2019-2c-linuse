#include "libMUSE.h"
#include <stdio.h>
#include <stdlib.h>
#include <biblioteca/paquetes.h>

int main(void) {
	return prueba();
}

uint32_t muse_alloc(uint32_t tam){

	uint32_t aux = malloc(tam);
	return aux;

}

void muse_free(uint32_t dir){

	 free(dir);

}

int muse_get(void* dst, uint32_t src, size_t n){

	return 1;
}

int muse_cpy(uint32_t dst, void* src, int n){

	int aux = memcpy(src,dst,n);
	return aux;
}

uint32_t muse_map(char *path, size_t length, int flags){

	uint32_t aux = mmap(path,length,0,flags,0,0);
	return aux;
}

int muse_sync(uint32_t addr, size_t len){

	int aux = msync(addr,len,0);
	return aux;
}

int muse_unmap(uint32_t dir){

	int aux = munmap(dir,0);
	return aux;

}


int pruebaReferenciaMUSE(void) {
	puts("!!!prueba biblite si lo camibas de nuevo !!!"); /* prints !!!Hello World!!! */
	return 1;
}
