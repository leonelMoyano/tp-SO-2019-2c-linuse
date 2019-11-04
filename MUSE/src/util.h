#ifndef util_h
#define util_h

#include "estructuras.h"
#include <biblioteca/estructurasAdministrativas.h>
#include <biblioteca/paquetes.h>


//si queres podemos cambiar parametros por estructuras que los contengan
void serializarUINT32(t_paquete* unPaquete, uint32_t numero);
int deserializarUINT32(t_stream* buffer);
void enviarRespuestaAlloc(int server_socket, uint32_t * tamanio);

#endif
