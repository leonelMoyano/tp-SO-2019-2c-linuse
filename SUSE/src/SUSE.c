/*
 ============================================================================
 Name        : suseServer.c
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 ============================================================================
 */

#include "SUSE.h"

void atenderConexion(int socketCliente);
t_paquete* procesarPaqueteLibSuse( t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente );
t_paquete* procesarThreadCreate(t_paquete* paquete, t_client_suse* cliente_suse, int is_main_thread, int socket_cliente);
t_paquete* procesarThreadClose(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
void       esperarPaqueteCreateMain( t_client_suse* cliente_suse, int socket_cliente );

int main(void)
{
	iniciar_logger();
	iniciar_config("/home/utnso/workspace/tp-2019-2c-No-C-Nada/configs/SUSE/suseServer.cfg");
	// TODO despues de levantar la config inicializar los semaforos de la config

	iniciarServidor(g_config_server->puerto, g_logger, (void*)atenderConexion);

	log_destroy(g_logger);
	config_destroy( g_config );
	return EXIT_SUCCESS;
}


void atenderConexion(int socketCliente) {
	log_debug(g_logger, "Attend connection con este socket %d", socketCliente);
	t_paquete* package = recibirArmarPaquete(socketCliente);
	t_paquete* response;

	log_debug(g_logger, "Checkeo que el paquete sea handshake");
	// Espero recibir el handshake y trato segun quien se conecte
	switch (recibirHandshake(package)) {
	case BIBLIO_SUSE_CLIENT_ID:
		;
		log_debug(g_logger, "Recibi el handshake del cliente");
		t_client_suse* cliente_suse = malloc( sizeof( t_client_suse ) );

		enviarMultiProg( socketCliente );
		esperarPaqueteCreateMain( cliente_suse, socketCliente );
		while (1) {
			package = recibirArmarPaquete(socketCliente);
			log_debug(g_logger, "Recibo paquete");

			if ( package == NULL || package->codigoOperacion == ENVIAR_AVISO_DESCONEXION ) {
				log_warning(g_logger, "Cierro esta conexion del LibMuse %d", socketCliente);
				break;
			};

			response = procesarPaqueteLibSuse(package, cliente_suse, socketCliente);
			if( response != NULL )
				enviarPaquetes(socketCliente, response);
		}
		break;
	default:
		log_warning(g_logger, "El paquete recibido no es handshake");
		break;
	}

	close(socketCliente);
}

t_paquete* procesarPaqueteLibSuse(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente) {
	log_debug(g_logger, "Proceso codigo op %d", paquete->codigoOperacion);
	t_paquete* response = NULL;

	switch (paquete->codigoOperacion) {
	/* nunca entra por aca porque el handshake lo recibo cuando entro a "attendConnection" */
	case SUSE_CREATE:
		response = procesarThreadCreate( paquete, cliente_suse, 0, socket_cliente );
		break;
	case SUSE_CLOSE:
		response = procesarThreadClose( paquete, cliente_suse, socket_cliente);
		break;
	case SUSE_JOIN:
		break;
	case SUSE_SCHEDULE_NEXT:
		break;
	case SUSE_SIGNAL:
		break;
	case SUSE_WAIT:
		break;
	}

	destruirPaquete( paquete );
	return response;
}

t_paquete* procesarThreadCreate(t_paquete* paquete, t_client_suse* cliente_suse, int is_main_thread, int socket_cliente){
	log_info( g_logger, "Recibi un create");

	int tid = deserializarNumero( paquete->buffer );
	t_client_thread_suse* nuevo_thread = malloc( sizeof( t_client_thread_suse ) );
	nuevo_thread->tid = tid;
	nuevo_thread->time_created = time( NULL );
	if( is_main_thread == 1 ){
		nuevo_thread->time_last_run = time( NULL );
		nuevo_thread->time_last_yield = time( NULL );

		cliente_suse->main_tid = tid;
		cliente_suse->running_thread = nuevo_thread;
		cliente_suse->new = queue_create();
		cliente_suse->waiting = list_create();
		cliente_suse->exit = list_create();
		cliente_suse->blocked = list_create();
	} else {
		// Le pongo 0 para simbolizar que nunca corrio
		nuevo_thread->time_last_run = 0;
		nuevo_thread->time_last_yield = 0;
		queue_push( cliente_suse->new, nuevo_thread );
	}

	return NULL;
}

t_paquete* procesarThreadClose(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO este deberia buscar el tid, hacer el memfree de su estrutura y pasarlo a la lista de exit
	return NULL;
}

void esperarPaqueteCreateMain( t_client_suse* cliente_suse, int socket_cliente ){
	t_paquete *paqueteCreate = recibirArmarPaquete( socket );
	if( paqueteCreate->codigoOperacion == SUSE_CREATE ){
		procesarThreadCreate( paqueteCreate, cliente_suse, 1, socket_cliente);
	} else {
		log_error( g_logger, "Recibi algo que no es el grado del multiprog");
	}
}

void enviarMultiProg( int socket_dst ){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));
	unPaquete->codigoOperacion = SUSE_GRADO_MULTIPROG;

	log_debug(g_logger, "Envio este grado de multiprogramacion %d", g_config_server->max_multiprog);
	serializarNumero(unPaquete, g_config_server->max_multiprog);
	enviarPaquetes(socket_dst, unPaquete);
}

void iniciar_config(char* path){
	g_config = config_create(path);
	g_config_server = malloc( sizeof( t_config_suse ) );

	g_config_server->puerto = config_get_string_value( g_config, "LISTEN_PORT" );
	g_config_server->metrics_timer = config_get_int_value( g_config, "METRICS_TIMER" );
	g_config_server->max_multiprog = config_get_int_value( g_config, "MAX_MULTIPROG" );

	char ** array = config_get_array_value( g_config, "SEM_IDS" );
	g_config_server->sem_ids = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_ids, array[ i ] );
	}

	array = config_get_array_value( g_config, "SEM_INIT" );
	g_config_server->sem_init = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_init, array[ i ] );
	}

	array = config_get_array_value( g_config, "SEM_MAX" );
	g_config_server->sem_max = list_create();
	for( int i = 0; array[ i ] != NULL; i++ ){
		list_add( g_config_server->sem_max, array[ i ] );
	}

	/*
	 * print (char *)list_get(configServer->sem_ids, 0 )
		$3 = 0x804ec20 "A"
		print atoi(list_get(g_config_server->sem_init, 0 ))
		$4 = 0
	 *
	 */
}

void iniciar_logger(void)
{
	g_logger = log_create("/home/utnso/workspace/tp-2019-2c-No-C-Nada/SUSE/logFiles/suseServer.log", "SUSE-Server", 1, LOG_LEVEL_INFO);
	log_info(g_logger, "Iniciando SuseServer");
}
