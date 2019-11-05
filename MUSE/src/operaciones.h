#ifndef operaciones_h
#define operaciones_h

#include "estructuras.h"


uint32_t procesarAlloc(uint32_t tam);

void procesarFree(uint32_t dir);

int procesarGet(void* dst, uint32_t src, size_t n);

int procesarCopy(uint32_t dst, void* src, int n);

uint32_t procesarMap(char *path, size_t length, int flags);

int procesarSync(uint32_t addr, size_t len);

int procesarUnMap(uint32_t dir);

#endif
