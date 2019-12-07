/* Lib implementation: It'll only schedule the last thread that was created */
#include "biblioSuse.h"

#define CONFIG_PATH "/home/utnso/workspace/confBiblioSuse/biblioSuse.cfg"
int g_max_multiprog; // Esta es la respuesta de grado de multiprogramacion en el server SUSE
int g_server_socket; // Esta es la referencia global al socket conectado al server SUSE

int suse_create(int tid) {
	// TODO hacer aca el chequeo real contra max multiprog del server
	if (tid > g_max_multiprog) {
		g_max_multiprog = tid;
	}
	printf("Se creo un nuevo hilo: %d...\n", tid);
	enviarThreadCreate( g_server_socket, tid );
	return 0;
}

int suse_schedule_next(void){
	int next = g_max_multiprog;
	printf("Scheduling next item %i...\n", next);
	//sendMssgSuse("Proximo hilo a ejecutar");
	return next;
}

int suse_join(int tid)
{
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
	// Not supported
	return 0;
}

int suse_signal(int tid, char *sem_name){
	// Not supported
	return 0;
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

void enviarThreadCreate(int socket, int tid) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SUSE_CREATE;

	serializarNumero(unPaquete, tid);

	enviarPaquetes(socket, unPaquete);
}

void esperarRespuestaConfig( int socket ){
	t_paquete *respuetaMultiprog = recibirArmarPaquete( socket );
	if( respuetaMultiprog->codigoOperacion == SUSE_GRADO_MULTIPROG ){
		g_max_multiprog = deserializarNumero( respuetaMultiprog->buffer );
	} else {
		log_error( g_logger, "Recibi algo que no es el grado del multiprog");
	}
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
	g_logger = log_create("/home/utnso/workspace/confBiblioSuse/suseServer.log", "biblioSuse", 1, LOG_LEVEL_INFO);
}
