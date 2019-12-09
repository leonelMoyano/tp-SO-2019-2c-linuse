/*
 ============================================================================
 Name        : suseServer.c
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 ============================================================================
 */

#include "SUSE.h"

void       atenderConexion(int socketCliente);
t_paquete* procesarPaqueteLibSuse( t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente );
t_paquete* procesarThreadCreate(t_paquete* paquete, t_client_suse* cliente_suse, int is_main_thread, int socket_cliente);
t_paquete* procesarThreadClose(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
t_paquete* procesarSemSignal(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
t_paquete* procesarSemWait(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
void       esperarPaqueteCreateMain( t_client_suse* cliente_suse, int socket_cliente );
void       inicializar_semaforos();
t_paquete* armarPaqueteNumeroConOperacion( int numero, int codigo_op );

int main(void)
{
	iniciar_logger();
	iniciar_config("/home/utnso/workspace/tp-2019-2c-No-C-Nada/configs/SUSE/suseServer.cfg");
	inicializar_semaforos();
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
		response = procesarSemSignal( paquete, cliente_suse, socket_cliente );
		break;
	case SUSE_WAIT:
		response = procesarSemWait( paquete, cliente_suse, socket_cliente );
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
	// TODO estimacion inicial en 0
	if( is_main_thread == 1 ){ // Cuando llega el create del main thread ya esta corriendo
		nuevo_thread->estado = RUNNING;
		nuevo_thread->time_last_run = time( NULL );
		nuevo_thread->time_last_yield = time( NULL );

		cliente_suse->main_tid = tid;
		cliente_suse->running_thread = nuevo_thread;
		cliente_suse->new = queue_create();
		cliente_suse->ready = list_create();
		cliente_suse->exit = list_create();
		cliente_suse->blocked = list_create();
	} else {
		// Le pongo 0 para simbolizar que nunca corrio
		nuevo_thread->estado = NEW;
		nuevo_thread->time_last_run = 0;
		nuevo_thread->time_last_yield = 0;

		// TODO revisar nivel max de multiprog y ponerlo a waiting y no a new ?
		queue_push( cliente_suse->new, nuevo_thread );
	}

	return NULL;
}

/**
* @NAME: trancisionar_bloqueado_a_ready
* @DESC: Transiciona el thread de bloqueado a ready, moviendolo de la cola de bloqueados a su cola de ready correspondiente
*/
void trancisionar_bloqueado_a_ready( t_client_thread_suse* thread ){
	bool compare_thread( t_client_thread_suse* otro_thread ){
		return otro_thread->tid == thread->tid;
	}

	// TODO cambiar la cola de bloqueado que agarro si va a terminar siendo una global para tods los procs
	list_remove_by_condition( thread->proceso_padre->blocked, compare_thread);
	list_add( thread->proceso_padre->ready, thread );
}

t_paquete* procesarThreadClose(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO este deberia buscar el tid pasarlo a la lista de exit
	// no hay que buscarlo realmente, siempre va a ser el que corriend, e.g. cliente_suse->running_thread

	// TODO pasar a ready los threads que esten bloqueados esperando el join
	// llamar a esto con la lista de thread_a_cerrar->threads_bloqueados
	// void list_iterate(t_list *, void(*closure)(void*));
	// usar esto para encontrar el tid del thread que desbloqueo dentro de los bloqueados
	// en thread_a_cerrar->proceso_padre
	// con un closure que los vaya moviendo de a uno

	return NULL;
}

/**
* @NAME: find_sem_by_name
* @DESC: Retorna el semaforo con nombre sem_name, NULL en caso de no existir
*/
t_semaforo_suse* find_sem_by_name( char* sem_name ){
	bool compare_sem_name( t_semaforo_suse* sem ){
		return strcmp( sem_name, sem->nombre ) == 0;
	}
	return list_find( g_semaforos, find_sem_by_name );
}

/**
* @NAME: find_thread_by_tid_in_parent
* @DESC: Retorna el thread con el tid buscando dentro del proceso padre, NULL en caso de no existir
* 	busco solo en RUNNING, READY, EXIT y BLOCKED, si esta en NEW no lo busco
*/
t_client_thread_suse* find_thread_by_tid_in_parent( int tid, t_client_suse* proceso_padre ){
	bool compare_thread_id( t_client_thread_suse* thread ){
		return tid == thread->tid;
	}
	// Lo busco en varios lados porque puede estar en cualquier estado
	if( proceso_padre->running_thread->tid == tid)
		return proceso_padre->running_thread;

	t_client_thread_suse* thread_buscado;

	thread_buscado = list_find( proceso_padre->ready, find_sem_by_name );
	if( thread_buscado != NULL )
		return thread_buscado;

	thread_buscado = list_find( proceso_padre->exit, find_sem_by_name );
	if( thread_buscado != NULL )
		return thread_buscado;

	return list_find( proceso_padre->blocked, find_sem_by_name );
}

t_paquete* procesarSemSignal(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO serializar y deserealizar el paquete para esto
	t_semaforo_request_suse* sem_req_info = deserializarSemaforoRequest( paquete->buffer );
	char* sem_name;
	int tid;

	t_paquete * respuesta = NULL;

	t_semaforo_suse* semaforo = find_sem_by_name( sem_name );

	if( semaforo == NULL ){
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_SIGNAL );
	} else if( semaforo->current_value == semaforo->max_value ) {
		respuesta = armarPaqueteNumeroConOperacion( -EOVERFLOW, SUSE_SIGNAL );
	} else {
		if( queue_is_empty( semaforo->threads_bloquedos ) )
			semaforo->current_value++;
		else{
			t_client_thread_suse* thread_desbloqueado = queue_pop( semaforo->threads_bloquedos );
			trancisionar_bloqueado_a_ready( thread_desbloqueado );
		}
		respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_SIGNAL );
	}
	return respuesta;
}

t_paquete* procesarSemWait(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO serializar y deserealizar el paquete para esto
	char* sem_name;
	int tid;

	t_paquete * respuesta = NULL;

	t_semaforo_suse* semaforo = find_sem_by_name( sem_name );
	t_client_thread_suse* running_thread = cliente_suse->running_thread; // Porque si hizo un wait tiene que estar en running

	if( semaforo == NULL ){
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_WAIT );
	} else if( semaforo->current_value == 0 ) {
		queue_push( semaforo->threads_bloquedos, running_thread );
		list_add( cliente_suse->blocked, running_thread );
	} else { // aca asumo que el semaforo puede ser restado porque no es 0
		semaforo->current_value--;
		respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_WAIT );
	}
	return respuesta;
}

t_paquete* armarPaqueteNumeroConOperacion( int numero, int codigo_op ){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigoOperacion = codigo_op;
	serializarNumero(paquete, numero);
	return paquete;
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

void inicializar_semaforos(){
	t_semaforo_suse* nuevo_semaforo;
	for( int i = 0; i < list_size( g_config_server->sem_ids ); i++ ){
		nuevo_semaforo = malloc( sizeof( t_semaforo_suse ) );
		nuevo_semaforo->nombre = strdup( list_get( g_config_server->sem_ids, i ) );
		nuevo_semaforo->current_value = atoi( list_get( g_config_server->sem_init, i ) );
		nuevo_semaforo->max_value = atoi( list_get( g_config_server->sem_max, i ) );
		nuevo_semaforo->threads_bloquedos = queue_create();
	}
}

void iniciar_logger(void)
{
	g_logger = log_create("/home/utnso/workspace/tp-2019-2c-No-C-Nada/SUSE/logFiles/suseServer.log", "SUSE-Server", 1, LOG_LEVEL_INFO);
	log_info(g_logger, "Iniciando SuseServer");
}
