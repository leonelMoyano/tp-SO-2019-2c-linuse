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
t_paquete* procesarThreadJoin(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
t_paquete* procesarThreadScheduleNext(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
t_paquete* procesarSemSignal(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
t_paquete* procesarSemWait(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente);
void       esperarPaqueteCreateMain( t_client_suse* cliente_suse, int socket_cliente );
void       inicializar_semaforos();
t_paquete* armarPaqueteNumeroConOperacion( int numero, int codigo_op );
t_client_thread_suse* find_thread_by_tid_in_parent( int tid, t_client_suse* proceso_padre );
double indice_sjf(t_client_thread_suse* thread_to_compare);

int main(void) {
	iniciar_logger();
	iniciar_config("/home/utnso/workspace/tp-2019-2c-No-C-Nada/configs/SUSE/suseServer.cfg");
	inicializar_estructuras();
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
				log_warning(g_logger, "Cierro esta conexion del cliente_suse %d", socketCliente);
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
		response = procesarThreadJoin( paquete, cliente_suse, socket_cliente);
		break;
	case SUSE_SCHEDULE_NEXT:
		response = procesarThreadScheduleNext( paquete, cliente_suse, socket_cliente);
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
	nuevo_thread->proceso_padre = cliente_suse;
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
void trancisionar_bloqueado_a_ready( void* thread ){
	t_client_thread_suse* thread_t = (t_client_thread_suse*) thread;
	bool compare_thread( void* otro_thread ){
		t_client_thread_suse* otro_thread_t = (t_client_thread_suse*) otro_thread;
		bool condition_1 = otro_thread_t->tid == thread_t->tid;
		bool condition_2 = otro_thread_t->proceso_padre->main_tid == thread_t->proceso_padre->main_tid;
		return condition_1 && condition_2;
	}
	// TODO cambiar la cola de bloqueado que agarro si va a terminar siendo una global para tods los procs
	list_remove_by_condition( g_blocked_threads, compare_thread);
	list_add( thread_t->proceso_padre->ready, thread );
}

void trancisionar_ready_a_bloqueado( void* thread ){
	t_client_thread_suse* thread_t = (t_client_thread_suse*) thread;
	bool compare_thread( void* otro_thread ){
		t_client_thread_suse* otro_thread_t = (t_client_thread_suse*) otro_thread;
		return otro_thread_t->tid == thread_t->tid;
	}
	list_remove_by_condition( thread_t->proceso_padre->ready, compare_thread);
	list_add( g_blocked_threads, thread );
	// pasamos el thread a la lista Global Blocked Threads
}

t_paquete* procesarThreadClose(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){

	log_info( g_logger, "Recibi un close");

	t_paquete* respuesta = NULL;

	if( cliente_suse->main_tid < 0){
		log_warning( g_logger, "Proceso %d inexistente en socket %d", cliente_suse->main_tid, socket_cliente );
		respuesta = armarPaqueteNumeroConOperacion( -ECHILD, SUSE_CLOSE );
	}
	else if ( cliente_suse->running_thread == NULL){
		log_warning( g_logger, "Proceso %d sin hilos en ejecucion en socket %d", cliente_suse->main_tid, socket_cliente );
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_CLOSE );
	}
	else{
		t_client_thread_suse* thread_a_cerrar = cliente_suse->running_thread;  	// obtengo el thread en ejecucion
		list_add(g_exit_threads,thread_a_cerrar);								// se agregó el running thread la lista Global EXIT Threads.
		thread_a_cerrar->estado = EXIT;											// cambio el estado a de *thread_a_cerrar* a EXIT.
		thread_a_cerrar->time_last_yield = time(NULL);							// actualizamos "time_last_yield" porque deja de ejecutarse
		t_list* pasar_a_ready = thread_a_cerrar->threads_bloqueados;			// en el "proceso_padre" de thread_a_cerrar
																				// muevo elementos de la lista "blocked" a "ready";
		list_iterate(pasar_a_ready, trancisionar_bloqueado_a_ready);            // contenidos en la lista "thread_bloqueados" de *thread_a_cerrar*
		list_clean(pasar_a_ready);
		/* porque una vez que transicionan a ready
		 * los tenemos que sacar de la cola de bloqueados del thread
		 * solo llamar a trancisionar_bloqueado_a_ready con cada elemento
		 * de la lista los saca de la lista de bloqueados del proceso pero quedan
		 * dentro de la lista de bloqueados por el thread */
		log_info( g_logger, "Operacion suse_close Ok para hilo en ejecucion %d del socket %d", cliente_suse->running_thread, socket_cliente );
		respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_CLOSE );
	}
	return respuesta;
}

t_paquete* procesarThreadJoin(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){

	log_info( g_logger, "Recibi un join");
	int tid_to_join = deserializarNumero( paquete->buffer );
	t_paquete* respuesta = NULL;

	if( cliente_suse->main_tid < 0){
		log_warning( g_logger, "Proceso %d inexistente en socket %d", cliente_suse->main_tid, socket_cliente);
		respuesta = armarPaqueteNumeroConOperacion( -ECHILD, SUSE_JOIN );
	}
	if ( cliente_suse->running_thread == NULL){
		log_warning( g_logger, "Proceso %d sin hilos en ejecucionen socket %d", cliente_suse->main_tid, socket_cliente );
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_JOIN );
	}
	else{
		t_client_thread_suse* thread_buscado_en_exit;
		bool compare_thread_in_thread_lists( void* thread ){
			t_client_thread_suse* thread_t = (t_client_thread_suse*) thread;
			bool condition_1 = tid_to_join == thread_t->tid;
			bool condition_2 = cliente_suse->main_tid == thread_t->proceso_padre->main_tid;
			return condition_1 && condition_2;
		}
		thread_buscado_en_exit = list_find( g_exit_threads, compare_thread_in_thread_lists );
		// busco en la Lista Global de EXIT threads el TID enviado por el request SUSE_JOIN (diferente al TID del running_thread).

		if (thread_buscado_en_exit != NULL) {
		//encontré al thread requested to join la Lista Global de EXIT threads
			log_info( g_logger, "Operacion Suse_Join Ok, Hilo %d de socket %d ya en EXIT", tid_to_join, socket_cliente );
			respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_JOIN );
		}
		else {
			cliente_suse->running_thread->estado = BLOCKED;													// le seteamos el estado BLOCKED al thread
			cliente_suse->running_thread->time_last_yield = time(NULL);									// actualizamos "time_last_yield" porque va a dejar de ejecutarse
			void* runningThread = cliente_suse->running_thread;
			list_add(g_blocked_threads,runningThread);														// agregamos al thread en ejecución a la lista "cliente_suse->blocked" del proceso que hizo el request
			t_client_thread_suse* thread_to_join = find_thread_by_tid_in_parent(tid_to_join, cliente_suse); // busco el thread que fue requerido a hacer Join
			list_add(thread_to_join->threads_bloqueados,runningThread); 									// agregamos a la lista "thread_bloqueados" del thread solicitado a hacer Join el running_thread al thread
			log_info( g_logger, "Operacion Suse_Join Ok, Hilo en ejecucion %d a BLOCKED del socket %d", cliente_suse->running_thread, socket_cliente );

			respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_JOIN );
		}

	}
	return respuesta;
}

t_paquete* procesarThreadScheduleNext(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){

	log_info( g_logger, "Recibi un Schedule_Next");
	t_paquete* respuesta = NULL;

	if( cliente_suse->main_tid < 0){
		log_warning( g_logger, "Proceso %s inexistente", cliente_suse->main_tid );
		respuesta = armarPaqueteNumeroConOperacion( -ECHILD, SUSE_SCHEDULE_NEXT);
	}
	if ( cliente_suse->running_thread == NULL){
		log_warning( g_logger, "Proceso %s sin hilos en ejecucion", cliente_suse->main_tid );
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_SCHEDULE_NEXT );
	}
	else {
		t_client_thread_suse* next_running_thread;
		t_client_thread_suse* prev_running_thread = cliente_suse->running_thread;

		bool comparo_threads_por_indice_sjf( void* thread, void* otro_thread ){
			t_client_thread_suse* thread_t = (t_client_thread_suse*) thread;
			t_client_thread_suse* otro_thread_t = (t_client_thread_suse*) otro_thread;
			return indice_sjf(thread_t) <= indice_sjf(otro_thread_t);
		}

		list_sort(cliente_suse->ready,comparo_threads_por_indice_sjf); 	// ordeno la lista Ready del proceso que envió el request, los threads por indice_sjf, el de menor indice en 1° lugar
		next_running_thread = list_get(cliente_suse->ready, 0);			// obtengo el 1° thread de la lista Ready,
		list_remove(cliente_suse->ready, 0);							// quitamos el 1° thread de la lista Ready,
		list_add(cliente_suse->ready, prev_running_thread);				// agregamos el running_thread actual a la lista de Ready, se dejará de ejecutar,
		prev_running_thread->time_last_yield = time(NULL);				// actualizamos time_last_yield del thread que se dejará de ejecutar,
		cliente_suse->running_thread = next_running_thread;				// ponemos a ejecutar el thread seleccionado de la lista Ready,
		next_running_thread->time_last_run = time(NULL);				// actualizamos time_last_run del thread que entrará en ejecucion,
		int next_running_tid = next_running_thread->tid;

		log_info( g_logger, "Operacion suse_schedule_next Ok para Proceso %d, proximo TID a ejecutar %d", cliente_suse->main_tid , next_running_tid);
		respuesta = armarPaqueteNumeroConOperacion( next_running_tid , SUSE_SCHEDULE_NEXT ); // en la respuesta al request envío el TID del nuevo running thread,
	}
	return respuesta;
}

/*
	 * TODO
	 * misma idea que en el close, si llaman un join ( o cualquier operacion )
	 * sabes que el thread que hizo el request es el que esta corriendo
	 * buscar si el thread id al que hizo join esta en la lista de exit
	 * ( threads que ya terminaron )
	 * - si el thread ya habia terminado entonces no tengo que hacer nada
	 * - si el thread no esta en exit:
	 *   muevo el thread que hizo el request a la lista de bloqueados
	 *   de cliente_suse
	 *   muevo el thread que hizo el request a la lista de bloqueados
	 *   del thread al que haya hecho join,
	 *   ese thread puede estar en la lista de bloqueado o en la lista de ready
	 *   tengo una funcion que todavia no probe para hacer eso
	 *   t_client_thread_suse* find_thread_by_tid_in_parent( int tid, t_client_suse* proceso_padre )
	 *   fijate si te parece bien lo que esta escrito ahi y se puede usar
	 *   lo que podemos hacer es que el cliente haga el request solo enviando
	 *   el tid del thread al que quiere hacer join y desde aca ya sabemos quien lo pidio
	 *   porque tiene que ser el que esta en running y nos ahorramos tener que codear
	 *   una funcion mas de serializar y deserializar
	 */




/**
* @NAME: find_sem_by_name
* @DESC: Retorna el semaforo con nombre sem_name, NULL en caso de no existir
*/
t_semaforo_suse* find_sem_by_name( char* sem_name ){
	bool compare_sem_name( void* sem ){
		t_semaforo_suse* sem_t = (t_semaforo_suse*) sem;
		return strcmp( sem_name, sem_t->nombre ) == 0;
	}
	return list_find( g_semaforos, compare_sem_name );
}


/**
* @NAME: find_thread_by_tid_in_parent
* @DESC: Retorna el thread con el tid buscando dentro del proceso padre, NULL en caso de no existir
* 	busco solo en RUNNING, READY, EXIT y BLOCKED, si esta en NEW no lo busco
*/

t_client_thread_suse* find_thread_by_tid_in_parent( int tid, t_client_suse* proceso_padre ){

	bool compare_thread_id( void* thread ){
		t_client_thread_suse* thread_t = (t_client_thread_suse*) thread;
		return tid == thread_t->tid;
	}

	// Lo busco en varios lados porque puede estar en cualquier estado
	if( proceso_padre->running_thread->tid == tid)
		return proceso_padre->running_thread;

	t_client_thread_suse* thread_buscado;

	thread_buscado = list_find( proceso_padre->ready, compare_thread_id );
	if( thread_buscado != NULL )
		return thread_buscado;

	thread_buscado = list_find( proceso_padre->exit, compare_thread_id );
	if( thread_buscado != NULL )
		return thread_buscado;

	return list_find( proceso_padre->blocked, compare_thread_id );
}

t_paquete* procesarSemSignal(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO poner semaforos ( de verdad ) dentro de cada operacion de suse_semaforo
	t_semaforo_request_suse* sem_req_info = deserializarSemaforoRequest( paquete->buffer );
	log_info( g_logger, "Sem signal de %s, para tid %d", sem_req_info->name, sem_req_info->tid );

	t_paquete * respuesta = NULL;

	t_semaforo_suse* semaforo = find_sem_by_name( sem_req_info->name );

	if( semaforo == NULL ){
		log_warning( g_logger, "Sem %s no existe", sem_req_info->name );
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_SIGNAL );
	} else if( semaforo->current_value == semaforo->max_value ) {
		log_warning( g_logger, "Sem %s ya esta en su valor maximo %d", sem_req_info->name, semaforo->max_value );
		respuesta = armarPaqueteNumeroConOperacion( -EOVERFLOW, SUSE_SIGNAL );
	} else {
		if( queue_is_empty( semaforo->threads_bloquedos ) ) {
			semaforo->current_value++;
			log_info( g_logger, "Sem %s nuevo valor %d", sem_req_info->name, semaforo->current_value );
		} else {
			t_client_thread_suse* thread_desbloqueado = queue_pop( semaforo->threads_bloquedos );
			trancisionar_bloqueado_a_ready( thread_desbloqueado );
			log_info( g_logger, "Sem signal en %s desbloqueo tid %d", sem_req_info->name, thread_desbloqueado->tid );
		}
		respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_SIGNAL );
	}
	return respuesta;
}

t_paquete* procesarSemWait(t_paquete* paquete, t_client_suse* cliente_suse, int socket_cliente){
	// TODO poner semaforos ( de verdad ) dentro de cada operacion de suse_semaforo
	t_semaforo_request_suse* sem_req_info = deserializarSemaforoRequest( paquete->buffer );
	log_info( g_logger, "Sem wait de %s, para tid %d", sem_req_info->name, sem_req_info->tid );

	t_paquete * respuesta = NULL;

	t_semaforo_suse* semaforo = find_sem_by_name( sem_req_info->name );
	t_client_thread_suse* running_thread = cliente_suse->running_thread; // Porque si hizo un wait tiene que estar en running

	if( semaforo == NULL ){
		log_warning( g_logger, "Sem %s no existe", sem_req_info->name );
		respuesta = armarPaqueteNumeroConOperacion( -EINVAL, SUSE_WAIT );
	} else if( semaforo->current_value == 0 ) {
		log_info( g_logger, "Sem %s esta en 0, se bloquea el tid %d", sem_req_info->name, sem_req_info->tid );
		running_thread->estado = BLOCKED;
		queue_push( semaforo->threads_bloquedos, running_thread );
		list_add( cliente_suse->blocked, running_thread );//TODO
		respuesta = armarPaqueteNumeroConOperacion( 0, SUSE_WAIT );
	} else { // aca asumo que el semaforo puede ser restado porque no es 0
		semaforo->current_value--;
		log_info( g_logger, "Sem %s nuevo valor %d", sem_req_info->name, semaforo->current_value );
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
	t_paquete *paqueteCreate = recibirArmarPaquete( socket_cliente );
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

		list_add( g_semaforos, nuevo_semaforo );
	}
}

void inicializar_estructuras(){
	g_semaforos = list_create();
}

void iniciar_logger(void) {
	g_logger = log_create("/home/utnso/workspace/tp-2019-2c-No-C-Nada/SUSE/logFiles/suseServer.log", "SUSE-Server", 1, LOG_LEVEL_TRACE);
	log_info(g_logger, "Iniciando SuseServer");
}

double indice_sjf(t_client_thread_suse* thread_to_compare) {
	long ready_time = thread_to_compare->time_last_run - thread_to_compare->time_created;
	long exec_time = thread_to_compare->time_last_yield - thread_to_compare->time_last_run;
	double alfa = g_config_server->alpha_sjf;
	double comp_alfa = 1 - g_config_server->alpha_sjf;
	double valor_calculado = alfa * ready_time + comp_alfa * exec_time;
	return valor_calculado;
}
