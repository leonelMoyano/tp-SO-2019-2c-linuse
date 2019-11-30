/* Lib implementation: It'll only schedule the last thread that was created */
#include "biblioSuse.h"

char* confSuse = "/home/utnso/workspace/confBiblioSuse/biblioSuse.cfg";
int max_tid = 0;

int suse_create(int tid)
{
	if (tid > max_tid) {
		max_tid = tid;
	}
	printf("Se creo un nuevo hilo: %d...\n", tid);
	sendMssgSuse("Se creo un nuevo hilo");
	return 0;
}

int suse_schedule_next(void){
	int next = max_tid;
	printf("Scheduling next item %i...\n", next);
	//sendMssgSuse("Proximo hilo a ejecutar");
	return next;
}

int suse_join(int tid)
{
	if (tid > max_tid) {
		max_tid = tid;
	}
	printf("Esperando se cierra el hilo:%d \n", tid);
	sendMssgSuse("Esperando se cierra el hilo: %d");
	return 0;
}

int suse_close(int tid){
	printf("Haciendo close del hilo: %i\n", tid);
	//sendMssgSuse("Haciendo close del hilo");
	max_tid--;
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
		.suse_wait = &suse_wait};

void hilolay_init(void)
{
	init_internal(&operaciones);
	sendMssgSuse("Se Inicio un Proceso");
}

void sendMssgSuse(char* mssg) {
	iniciar_log();
	config = leer_config(confSuse);
	char* ip = getAddress(config);
	char* port = getPort(config);
	int  socketServer =	crear_conexion(ip, port);
	enviar_mensaje(mssg , socketServer);
	log_info(logger, "Socket servidor creado %d", socketServer);
	log_destroy(logger);
	config_destroy(config);
	close(socketServer);
}

void iniciar_log(void)
{
	logger = log_create("/home/utnso/workspace/confBiblioSuse/suseServer.log", "biblioSuse", 1, LOG_LEVEL_INFO);
}