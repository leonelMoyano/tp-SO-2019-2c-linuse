#ifndef operaciones_h
#define operaciones_h

#include "estructuras.h"
#include "manejoEstructuras.h"


uint32_t procesarAlloc(uint32_t tam, int socket);

void procesarFree(uint32_t dir, int socket);

int procesarGet(void* dst, uint32_t src, size_t n, int socket);

int procesarCopy(uint32_t dst, void* src, int n, int socket);

uint32_t procesarMap(char *path, size_t length, int flags, int socket);

int procesarSync(uint32_t addr, size_t len, int socket);

int procesarUnMap(uint32_t dir, int socket);

#endif
