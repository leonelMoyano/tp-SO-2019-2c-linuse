#ifndef util_h
#define util_h

#include <biblioNOC/estructurasAdministrativas.h>
#include <biblioNOC/paquetes.h>
#include "libMUSE.h"
#include <setjmp.h>
#include <signal.h>

//extern int pid;

//si queres podemos cambiar parametros por estructuras que los contengan
void serializarUINT32(t_paquete* unPaquete, uint32_t numero);
uint32_t deserializarUINT32(t_stream* buffer);

void serializarGet(t_paquete* unPaquete, void* dst,uint32_t src, size_t n);

void serialzarCopy(t_paquete* unPaquete, void* src,uint32_t dst, int n);

void serializarMap(t_paquete * unPaquete, char * path, size_t length, int flags);

void serializarMsync(t_paquete * unPaquete,uint32_t addr, size_t len);

void serializarUnmap(t_paquete * paquete, uint32_t dir);

void enviarMuseClose(int server_socket);
void enviarMuseInit(int server_socket);
void enviarAlloc(int server_socket, uint32_t tamanio);
void enviarFree(int server_socket, uint32_t direccionLogica);
void enviarGet(int server_socket,void* dst,  uint32_t src, size_t n);
void enviarCopy(int server_socket,void* src,  uint32_t dst, int n);
void enviarMap(int server_socket,char * path, size_t length, int flags);
void enviarMsync(int server_socket,uint32_t addr, size_t len);
void enviarUnmap(int server_socket, uint32_t dir);



#endif
