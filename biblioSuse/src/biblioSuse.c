/* Lib implementation: It'll only schedule the last thread that was created */
#include "biblioSuse.h"

#define CONFIG_PATH "/home/utnso/workspace/confBiblioSuse/biblioSuse.cfg"
int g_max_multiprog; // Esta es la respuesta de grado de multiprogramacion en el server SUSE
int g_server_socket; // Esta es la referencia global al socket conectado al server SUSE

int suse_create(int tid) {
	/*
	 *  No chequeo contra max nivel de multiprog porque eso solo indica la cantidad de threads
	 *  que pueden estar en runnign/waiting, en NEW puede haber muchos mas que el max nivel
	 */
	printf("Se creo un nuevo hilo: %d...\n", tid);
	enviarThreadCreate( g_server_socket, tid );
	return 0;
}

int suse_schedule_next(void) {
	// int next = g_max_multiprog;
	// printf("Scheduling next item %i...\n", next);
	// sendMssgSuse("Proximo hilo a ejecutar");
	log_debug( g_logger, "Suse schedule next devuelvo siempre 0");
	return 0;
}

int suse_join(int tid) {
	if (tid > g_max_multiprog) {
		g_max_multiprog = tid;
	}
	printf("Esperando se cierra el hilo:%d \n", tid);
	sendMssgSuse("Esperando se cierra el hilo: %d");
	return 0;
}

int suse_close(int tid){
	printf("Haciendo close del hilo: %i\n", tid);
	//sendMssgSuse("Haciendo close del hilo");
	g_max_multiprog--;
	return 0;
}

int suse_wait(int tid, char *sem_name){
	log_info( g_logger, "Sem wait de %s, para tid %d", sem_name, tid );
	enviarSemWait( g_server_socket, tid, sem_name );
	return esperarRespuestaSemWait( g_server_socket );
}

int suse_signal(int tid, char *sem_name){
	log_info( g_logger, "Sem post de %s, para tid %d", sem_name, tid );
	enviarSemPost( g_server_socket, tid, sem_name );
	return esperarRespuestaSemPost( g_server_socket );
}

struct hilolay_operations operaciones = {
		.suse_close = &suse_close,
		.suse_create = &suse_create,
		.suse_join = &suse_join,
		.suse_schedule_next = &suse_schedule_next,
		.suse_signal = &suse_signal,
		.suse_wait = &suse_wait
};

void hilolay_init(void) {
	init_config( CONFIG_PATH );
	iniciar_log();

	log_info( g_logger, "Me conecto a SUSE server en %s:%s", g_config->ip, g_config->puerto );
	g_server_socket = conectarCliente( g_config->ip, atoi( g_config->puerto ), BIBLIO_SUSE_CLIENT_ID);
	esperarRespuestaConfig( g_server_socket );
	init_internal(&operaciones);
}

void init_config(char *path){
	g_config_commons = config_create(path);
	g_config = malloc( sizeof( t_config_lib_suse ) );

	g_config->ip = config_get_string_value( g_config_commons, "IP" );
	g_config->puerto = config_get_string_value( g_config_commons, "PUERTO" );
}

void enviarThreadCreate(int socket_dst, int tid) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SUSE_CREATE;

	serializarNumero(unPaquete, tid);

	enviarPaquetes(socket_dst, unPaquete);
}

void enviarSemWait( int socket_dst, int tid, char* nombre ){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SUSE_WAIT;

	serializarSemaforoRequest(unPaquete, tid, nombre);

	enviarPaquetes(socket_dst, unPaquete);
}

void enviarSemPost( int socket_dst, int tid, char* nombre ){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SUSE_SIGNAL;

	serializarSemaforoRequest(unPaquete, tid, nombre);

	enviarPaquetes(socket_dst, unPaquete);
}

void esperarRespuestaConfig( int socket ){
	t_paquete *respuetaMultiprog = recibirArmarPaquete( socket );
	if( respuetaMultiprog->codigoOperacion == SUSE_GRADO_MULTIPROG ){
		g_max_multiprog = deserializarNumero( respuetaMultiprog->buffer );
		// TODO free de este paquete ?
		log_info( g_logger, "Recibi este grado de multiprog: %d", g_max_multiprog );
	} else {
		log_error( g_logger, "Recibi algo que no es el grado del multiprog");
	}
}

int esperarRespuestaSemWait( int socket ){
	t_paquete *respuetaMultiprog = recibirArmarPaquete( socket );
	if( respuetaMultiprog->codigoOperacion != SUSE_WAIT ){
		log_error( g_logger, "Recibi algo que no es respuesta de sem wait");
	}
	int respuesta = deserializarNumero( respuetaMultiprog->buffer );
	log_info( g_logger, "Sem wait recibio esta respuesta %d", respuesta );
	destruirPaquete( respuetaMultiprog );
	return respuesta;
}

int esperarRespuestaSemPost( int socket ){
	t_paquete *respuetaMultiprog = recibirArmarPaquete( socket );
	if( respuetaMultiprog->codigoOperacion != SUSE_SIGNAL ){
		log_error( g_logger, "Recibi algo que no es respuesta de sem signal");
	}
	int respuesta = deserializarNumero( respuetaMultiprog->buffer );
	log_info( g_logger, "Sem signal recibio esta respuesta %d", respuesta );
	destruirPaquete( respuetaMultiprog );
	return respuesta;
}

void sendMssgSuse(char* mssg) {
	char* ip = g_config->ip;
	char* port = g_config->puerto;
	int  socketServer =	crear_conexion(ip, port);
	enviar_mensaje(mssg , socketServer);
	log_info(g_logger, "Socket servidor creado %d", socketServer);
	log_destroy(g_logger);
	close(socketServer);
}

void iniciar_log(void) {
	g_logger = log_create("/home/utnso/workspace/confBiblioSuse/suseServer.log", "biblioSuse", 1, LOG_LEVEL_TRACE);
}
