// -- FUSE import y boilerplate
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>

// -- SAC-CLI imports
#include <commons/temporal.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <biblioNOC/paquetes.h>
#include "sac.h"
#include "fuse_utils.h"
#include "sac_cod_ops.h"
#include "fuse_serializaciones.h"

t_log* g_logger;

ptrGBloque* g_first_block; // Puntero al inicio del primer bloque dentro del archivo
long g_disk_size; // Tamanio en bytes del archivo
GHeader* g_header; // Puntero al header del FS
GFile* g_node_table; // Puntero al primer nodo del FS
int g_node_table_block_index; // Indice de comienzo de la tabla de nodos
u_int32_t g_block_count; // Cantidad de bloques en el archivo
t_bitarray* g_bitmap; // Puntero al bitmap del FS

/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE
 */
struct t_runtime_options {
} runtime_options;

/*
 * Esta Macro sirve para definir nuestros propios parametros que queremos que
 * FUSE interprete. Esta va a ser utilizada mas abajo para completar el campos
 * welcome_msg de la variable runtime_options
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


/**
* @NAME: find_by_name_in_parent
* @DESC: Devuelve el indice en la tabla de nodos para el archivo de nombre name con nodo padre parent_index, -1 en caso de no encontrarlo
*
*/
int find_by_name_in_parent(const char *name, int parent_index){
	GFile* currNode;
	for(int currNodeIndex = 0; currNodeIndex < GFILEBYTABLE; currNodeIndex++){
		currNode = g_node_table + currNodeIndex;
		if( currNode->state != 0 && currNode->parent_dir_block == parent_index && strcmp( name, currNode->fname ) == 0 ){
			return currNodeIndex;
		}
	}
	return -1;
}


/**
* @NAME: find_by_path
* @DESC: Devuelve el indice en la tabla de nodos para el archivo con full path igual a path, -1 en caso de no encontrarlo
*
*/
int find_by_path(const char *path){
	int parent_node_index = get_parent_node( path );

	int splitted_path_index = 0;
	char **splitted_path = string_split( path, "/" );
	while( splitted_path[ splitted_path_index + 1 ] != NULL ) {
		free( splitted_path[ splitted_path_index ] );
		splitted_path_index++;
	}
	int nodo_encontrado = find_by_name_in_parent( splitted_path[ splitted_path_index ], parent_node_index );
	free( splitted_path[ splitted_path_index ] );
	return nodo_encontrado;
}

static int do_centralized_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	log_info( g_logger, "[readdir]:%s", path );

	t_paquete* request = armarPaquetePathConOperacion( path, SAC_readdir );
	t_paquete* response = send_request( request, "127.0.0.1", "54660" );

	t_readdir_response* readdir_response = deserializarReaddir( response->buffer );
	destruirPaquete( response );

	if( readdir_response->errno_value != 0 ){
		int errno_value = readdir_response->errno_value;
		destruirReaddirResponse( readdir_response );

		errno = errno_value;
		return -errno_value;
	}

	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory

	for( int i = 0; i < readdir_response->count; i++ ){
		filler(buffer, readdir_response->found[ i ], NULL, 0);
	}

	destruirReaddirResponse( readdir_response );
	return 0;
}

static int do_centralized_truncate(const char* path, off_t size){
	log_info( g_logger, "[truncate]:%s a size %ld", path, size );

	t_paquete* paquete = armarPaqueteTruncate( path, size, SAC_truncate );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

static int do_centralized_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	log_info( g_logger, "[read]:%s", path );

	t_paquete* paquete = armarPaqueteRead( path, size, offset, SAC_read );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_read_response* read_response = deserializarRead( response->buffer );

	if( read_response->errno_value != 0 ){
		int errno_value = read_response->errno_value;
		errno = errno_value;
		destruirPaquete( response );
		destruirReadResponse( read_response );
		return -errno_value;
	}

	int copy_size = read_response->size;
	memcpy( buffer, read_response->data, copy_size );

	destruirPaquete( response );
	destruirReadResponse( read_response );
	return copy_size;
}


/**
* @NAME: get_avail_node
* @DESC: Devuelve el indice en la tabla de nodos del proximo nodo libre, -EDQUOT
*
*/
int get_avail_node(){
	int currNode = 0;

	while( g_node_table[currNode].state!=0 && currNode < GFILEBYTABLE ) // Busco la proxima entrada disponible en la tabla de nodos
		currNode++;

	if (currNode >= g_block_count)
		return -EDQUOT; // No tengo nodos disponibles para crear otro archivo
	return currNode;
}


/**
* @NAME: get_parent_node
* @DESC: Devuelve el indice absoluto del nodo padre para path, 0 si es root, -ENOTDIR si no existe el directorio
*
*/
int get_parent_node(const char *path){
	// TODO liberar la memoria del split
	char **split_path = string_split( path, "/" );
	// string_split( "/testetset/opt/okok", "/" ) -> [ testsetest, opt, okokok, NULL ]
	// string_split( "/", "/" ) -> [ NULL ]
	// string_split( "/test", "/" ) -> [ test, NULL ]
	int currentParent = 0;
	int currentPathIndex = 0;

	GFile *currNode;

	while( split_path[ currentPathIndex ] != NULL && split_path[ currentPathIndex + 1 ] != NULL ){
		// Sumo y resto el indice donde inicia la tabla de nodos porque find_by_name_in_parent
		// devuelve el indice relativo dentro de la tabla y los padres estan indicados como indices absolutos
		currentParent = find_by_name_in_parent( split_path[ currentPathIndex ], currentParent ) + g_node_table_block_index;
		currNode = g_node_table + currentParent - g_node_table_block_index;
		if( currNode->state != 2 )
			return -ENOTDIR;
		currentPathIndex++;
	}
	return currentParent;
}


/**
* @NAME: check_existance_and_availability
* @DESC: Chequea la existencia de path, si existe devuelve -EEXIST,
* si no existe devuelve indice a un nodo libre en la tabla,
* si no hay nodos libres devuelve -EDQUOT
*
*/
int check_existance_and_availability( const char *path ){
	int currNode = find_by_path(path);
	if( currNode != -1 ){
		return -EEXIST; // path already exists
	}

	currNode = get_avail_node();
	if( currNode == -EDQUOT )
		return -EDQUOT;
	return currNode;
}


void occupy_node( char *path, int nodeIndex, int parentNodeIndex, int state ){
	GFile* nodeToSet = g_node_table + nodeIndex;

	char **splitted_path = string_split( path, "/" );
	int splitted_path_index = 0;
	// TODO liberar memoria de string_split
	while( splitted_path[ splitted_path_index + 1 ] != NULL )
		splitted_path_index++;

	strcpy((char*) nodeToSet->fname, splitted_path[ splitted_path_index ] );
	nodeToSet->state = state;
	nodeToSet->file_size = 0;
	nodeToSet->parent_dir_block = parentNodeIndex;
	nodeToSet->c_date = time( NULL );

	memset( nodeToSet->blk_indirect, 0, BLKINDIRECT );
}

static int do_centralized_getattr(const char *path, struct stat *st) {
	log_info( g_logger, "[getattr]:%s", path );

	t_paquete* request = armarPaquetePathConOperacion( path, SAC_getattr );
	t_paquete* response = send_request( request, "127.0.0.1", "54660" );

	t_getattr_response* getattr_response = deserializarGetattr( response->buffer );
	destruirPaquete( response ); // este se puede free de una porque solo saca valores

	if( getattr_response->type == 0 ){
		free( getattr_response );
		errno = ENOENT;
		return -ENOENT;
	}

	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now

	if( getattr_response->type == 1 )
		st->st_mode = S_IFREG | 0644;
	else
		st->st_mode = S_IFDIR | 0755;

	st->st_nlink = 1;
	st->st_mtime = getattr_response->last_mod_time; // The last "m"odification of the file/directory is right now
	st->st_size = getattr_response->size;

	free( getattr_response );
	return 0;
}

static int do_centralized_mknod(const char *path, mode_t mode, dev_t device){
	log_info( g_logger, "[mknod]:%s", path );

	t_paquete* paquete = armarPaquetePathConOperacion( path, SAC_mknod );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

static int do_centralized_utimens( const char *path, const struct timespec tv[2]){
	log_info( g_logger, "[utimens]:%s", path );

	// donde tv[ 0 ] es last access time y tv[ 1 ] es last modified time
	// solo existe last modified time en SAC asi que no tengo fecha de ultimo acceso que actualizar
	t_paquete* paquete = armarPaqueteUtimens( path, tv[0].tv_sec, SAC_utimens );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

static int do_centralized_mkdir(const char *path, mode_t mode){
	log_info( g_logger, "[mkdir]:%s", path );
	t_paquete* paquete = armarPaquetePathConOperacion( path, SAC_mkdir );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

static int do_centralized_unlink (const char *path){
	log_info( g_logger, "[unlink]:%s", path );

	t_paquete* paquete = armarPaquetePathConOperacion( path, SAC_unlink );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

static int do_centralized_rmdir( const char *path ){
	log_info( g_logger, "[rmdir]:%s", path );

	t_paquete* paquete = armarPaquetePathConOperacion( path, SAC_rmdir );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}


/**
* @NAME: seek_offset
* @DESC: Devuelve la direccion de memoria para un offset dentro de un archivo
*
*/
uint32_t seek_offset( GFile* fileNode, off_t offset ){
	int indirectBlockIndex = get_indirect_block_index(offset);
	int dataBlockIndexInsideIndirectBlock = get_datablock_index_inside_indirect_block(offset);
	int localBlockOffset = offset % GBLOCKSIZE;

	GPtrIndSimple* indirectBlock = (GPtrIndSimple*)g_header + fileNode->blk_indirect[ indirectBlockIndex ];
	GDataBlock* dataBlock = (GDataBlock*)g_header + indirectBlock->blk_direct[dataBlockIndexInsideIndirectBlock];
	return (uint32_t)(dataBlock->bytes + localBlockOffset);
}


int get_datablock_index(off_t offset) {
	return offset / GBLOCKSIZE;
}

int get_indirect_block_index(off_t offset) {
	return get_datablock_index(offset) / PTRBYINDIRECT;
}

int get_datablock_index_inside_indirect_block(off_t offset) {
	return get_datablock_index(offset) % PTRBYINDIRECT;
}

size_t min( size_t a, size_t b ){
	return a < b ? a : b;
}

/**
* @NAME: get_avail_block
* @DESC: Devuelve el indice dentro de todx el archivo del primer bloque libre, 0 si no hay bloques libres
*
*/
uint32_t get_avail_block(){
	for( int i  = 0; i < g_block_count; i++){
		if( !bitarray_test_bit(g_bitmap, i)){
			return i;
		}
	}
	return 0;
}

/**
* @NAME: copy_file_contents
* @DESC: Copia desde offset en el archivo todx el size. Si read_mode == 1 copia desde el archivo al buffer ( para read )
* , si read_mode != 1 copia desde el buffer al archivo ( para write )
*
*/
void copy_file_contents(GFile* fileNode, const char *buffer, size_t size, off_t offset, int read_mode){
	uint32_t filePointer = seek_offset( fileNode, offset ); // filePointer no es un buen nombre pero esto representa el puntero al offset dentro del archivo

	int dataBlockIndex = get_datablock_index(offset);

	size_t pendingSize = size;
	size_t copySize;
	size_t spaceLeftOnBlock = GBLOCKSIZE - (offset % GBLOCKSIZE);

	while (pendingSize) {
		copySize = min(min( GBLOCKSIZE, spaceLeftOnBlock), pendingSize);

		if(read_mode == 1) {
			// read, copio del archivo al buffer
			memcpy((char *)(buffer + size - pendingSize), (void*)filePointer, copySize);
		} else {
			// write, copio del buffer al archivo
			memcpy((void*)filePointer, buffer + size - pendingSize, copySize);
		}

		spaceLeftOnBlock = GBLOCKSIZE; // porque el proximo bloque lo voy a tener desde el principio
		pendingSize -= copySize;
		// muevo el filePointer a principio de proximo bloque ( offset 4096 es el byte 0 del block 1 )
		dataBlockIndex++;
		if (pendingSize)
			// si estaba apuntando al ultimo bloque ( 1000 * 1024 ) tal vez tenga comportamiento raro
			filePointer = seek_offset(fileNode, dataBlockIndex * GBLOCKSIZE);
	}
}

/**
* @NAME: get_occupied_datablocks_qty
* @DESC: Devuelve la cantidad de bloques de datos que necesito para guardar size bytes
*
*/
int get_occupied_datablocks_qty(size_t size){
	return size == 0 ? 0 : get_datablock_index(size - 1) + 1;
}

/**
* @NAME: get_occupied_ind_blocks_qty
* @DESC: Devuelve la cantidad de bloques indirectos que necesito para guardar size bytes
*
*/
int get_occupied_ind_blocks_qty(size_t size){
	return size == 0 ? 0 : get_indirect_block_index(size - 1) + 1;
}

/**
* @NAME: get_occupied_total_blocks_qty
* @DESC: Devuelve la cantidad de bloques totales que necesito para guardar size bytes
*
*/
int get_occupied_total_blocks_qty(size_t size){
	return get_occupied_datablocks_qty( size ) + get_occupied_ind_blocks_qty( size );
}

/**
* @NAME: get_free_blocks
* @DESC: Devuelve la cantidad de bloques vacios en el FS
*
*/
int get_free_blocks(){
	int free = 0;
	for( int i  = 0; i < g_block_count; i++ ){
		if( !bitarray_test_bit(g_bitmap, i))
			free ++ ;
	}
	return free;
}

/**
* @NAME: is_enough_blocks
* @DESC: Devuelve 1 si tengo la cantidad de bloques suficiente para agrandar un archivo de size,
*        0 si no tengo bloques suficientes
*
*/
bool is_enough_blocks( size_t current_size, size_t new_size ){
	int needed_blocks = get_occupied_total_blocks_qty( new_size ) - get_occupied_total_blocks_qty( current_size );
	return get_free_blocks() >= needed_blocks ? 1 : 0;
}

static int do_centralized_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi){
	log_info( g_logger, "[write]:%s", path );

	t_paquete* paquete = armarPaqueteWrite( path, size, offset, buffer, SAC_write );
	t_paquete* response = send_request( paquete, "127.0.0.1", "54660" );

	t_return_errno_response* errno_response = deserializarReturnErrno( response->buffer );

	if( errno_response->return_value < 0 )
		errno = errno_response->errno_value;

	int return_value = errno_response->return_value;

	destruirPaquete( response );
	free( errno_response );

	return return_value;
}

/**
* @NAME: liberarBloques
* @DESC: Libera los bloques de datos que sobren restando cantidadDatablocksActuales - cantidadDatablocksFinales.
* Devuelve la cantidad de datablocks liberados.
*
*/
int liberarBloques(GFile* fileNode, int cantidadDatablocksActuales, int cantidadDatablocksFinales){
	// TODO poner semaforos
	// Resto 1 porque la cantidad la paso como 1 padded y para sacar los indices necesito 0 padded
	int datablockIndexInsideIndir = ( cantidadDatablocksActuales -1 ) % PTRBYINDIRECT;
	int indirIndex = ( cantidadDatablocksActuales - 1 ) / PTRBYINDIRECT;
	GPtrIndSimple* ptrIndir;
	int freedDatablocks = 0;
	for(int i = 0; i < cantidadDatablocksActuales - cantidadDatablocksFinales; i++){
		ptrIndir = (GPtrIndSimple*)g_header + fileNode->blk_indirect[ indirIndex ];
		bitarray_clean_bit(g_bitmap, ptrIndir->blk_direct[datablockIndexInsideIndir]);
		freedDatablocks++;
		datablockIndexInsideIndir--;
		// Si borre el 0 y tengo que borrar uno mas agarro el ultimo del indirecto anterior
		if( datablockIndexInsideIndir == -1 ){
			freedDatablocks++;
			bitarray_clean_bit(g_bitmap, fileNode->blk_indirect[indirIndex]);
			datablockIndexInsideIndir = PTRBYINDIRECT - 1;
			indirIndex--;
		}
	}
	log_debug( g_logger, "Libero %d bloques", freedDatablocks );
	return freedDatablocks;
}

/**
* @NAME: reservarBloques
* @DESC: Reserva los bloques de datos necesarios restando cantidadDatablocksFinales - cantidadDatablocksActuales.
* Devuelve la cantidad de datablocks liberados.
*
*/
int reservarBloques( GFile* fileNode, int cantBlocksActuales, int cantBlocksFinales ){
	// TODO poner semaforos
	int reservedDatablocks = 0;

	int bloquesFaltantes = cantBlocksFinales - cantBlocksActuales;

	int nuevoDatablock;
	int nuevoIndirect;
	int datablockIndiceActual = cantBlocksActuales - 1;
	int proxDatablock = cantBlocksActuales == 0 ? 0 : datablockIndiceActual % 1024 + 1;
	int proxIndirect = cantBlocksActuales == 0 ? 0 : datablockIndiceActual / 1024 + 1;

	GPtrIndSimple* indirectBlock;
	// tal vez valga la pena reescribir esto para no tener el proximo indice sino el actual y que sea menos confuso ?

	for (int i = 0; i < bloquesFaltantes; i++) {
		// para que agarre tanto cuando tengo 0 como cuando llego a 1024
		if( proxDatablock % 1024 == 0 ){
			// necesito un bloque indirecto
			nuevoIndirect = get_avail_block();
			memset( (GPtrIndSimple*)g_header + nuevoIndirect, 0, GBLOCKSIZE ); // para limpiar el bloque que voy a usar como ind
			fileNode->blk_indirect[proxIndirect] = nuevoIndirect;
			bitarray_set_bit(g_bitmap, nuevoIndirect);
			proxIndirect++;
			proxDatablock = 0;
			reservedDatablocks++;
		}
		nuevoDatablock = get_avail_block();
		indirectBlock = (GPtrIndSimple*) g_first_block + fileNode->blk_indirect[proxIndirect - 1];
		indirectBlock->blk_direct[ proxDatablock ] = nuevoDatablock;
		bitarray_set_bit(g_bitmap, nuevoDatablock);
		proxDatablock++;
		reservedDatablocks++;
	}

	return reservedDatablocks;
}

static int do_rename(const char *old_path, const char *new_path){
	log_info( g_logger, "[rename]:%s a %s", old_path, new_path );
	int currNodeIndex = find_by_path( old_path );
	if( currNodeIndex == -1 ){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;
	int new_parent_node = get_parent_node( new_path );

	currNode->parent_dir_block = new_parent_node;

	char **splitted_path = string_split( new_path, "/" );
	int splitted_path_index;
	for(splitted_path_index = 0; splitted_path[ splitted_path_index + 1 ] != NULL; splitted_path_index++ ){
		free(splitted_path[ splitted_path_index ]);
	}

	strcpy((char*) currNode->fname, splitted_path[ splitted_path_index ] );
	free( splitted_path[ splitted_path_index ] );

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return 0;
}

static struct fuse_operations operations = {
	.getattr  = do_centralized_getattr,
	.mknod    = do_centralized_mknod,
	.mkdir    = do_centralized_mkdir,
	.utimens  = do_centralized_utimens,
	.unlink   = do_centralized_unlink,
	.rmdir    = do_centralized_rmdir,
	.readdir  = do_centralized_readdir,
	.truncate = do_centralized_truncate,
	.read     = do_centralized_read,
	.write    = do_centralized_write,
	/*
	.rename = do_rename,
	*/
};

/** keys for FUSE_OPT_ options */
enum {
	KEY_VERSION,
	KEY_HELP,
};

/*
 * Esta estructura es utilizada para decirle a la biblioteca de FUSE que
 * parametro puede recibir y donde tiene que guardar el valor de estos
 */
static struct fuse_opt fuse_options[] = {
		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	g_logger = log_create( "/home/utnso/logs/FUSE/cli.log", "SACCLI", 1, LOG_LEVEL_TRACE );
	// TODO levantar config, crear struct y var global
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		log_error( g_logger, "Invalid fuse arguments" );
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todx
	// en varios threads
	return fuse_main(args.argc, args.argv, &operations, NULL);
}

// Funciones para conectar una sola ves por socket, request y, liberar

t_paquete* send_request( t_paquete* request, char* ip, char* puerto ){
	int server_socket = conectarCliente( ip, atoi( puerto ), SAC_CLI );

	enviarPaquetes( server_socket, request );
	t_paquete* respuesta = recibirArmarPaquete( server_socket );

	enviarAvisoDesconexion( server_socket );
	close( server_socket );
	return respuesta;
}
