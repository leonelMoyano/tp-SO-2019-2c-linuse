/*
 * sac_fs_handlers.c
 *
 *  Created on: 16 dic. 2019
 *      Author: utnso
 */

// -- FUSE import y boilerplate
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
#include <commons/log.h>
#include <commons/bitarray.h>
#include <biblioNOC/paquetes.h>
#include "sac.h"
#include "fuse_utils.h"

#include "sac_cod_ops.h"
#include "sac_fs_handlers.h"
#include "sac_server_serializaciones.h"

t_log* g_logger;

ptrGBloque* g_first_block; // Puntero al inicio del primer bloque dentro del archivo
long g_disk_size; // Tamanio en bytes del archivo
GHeader* g_header; // Puntero al header del FS
GFile* g_node_table; // Puntero al primer nodo del FS
int g_node_table_block_index; // Indice de comienzo de la tabla de nodos
u_int32_t g_block_count; // Cantidad de bloques en el archivo
t_bitarray* g_bitmap; // Puntero al bitmap del FS


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

t_paquete* procesar_getattr( t_paquete* request ) {
	char* path = deserializarMensaje( request->buffer );

	log_info( g_logger, "[getattr]:%s", path );

	if (strcmp(path, "/") == 0) {
		free( path );
		return armarPaqueteGetattr( 2, time( NULL ), 0, SAC_getattr );
	}

	int currNodeIndex = find_by_path(path);
	free( path );

	if (currNodeIndex != -1) {
		GFile* currNode = g_node_table + currNodeIndex;

		return armarPaqueteGetattr( currNode->state, currNode->m_date, currNode->file_size, SAC_getattr );
	}

	return armarPaqueteGetattr( 0, 0, 0, SAC_getattr );
}
/*
static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	log_info( g_logger, "[readdir]:%s", path );

	int parentNodeIndex;
	if( strcmp( path, "/" ) == 0 ){
		parentNodeIndex = 0;
	} else {
		parentNodeIndex = find_by_path( path );
		if( parentNodeIndex == -1 )
			return -ENOENT;
		else {
			GFile *parentNode = g_node_table + parentNodeIndex;
			if( parentNode->state == 1 )
				return -ENOTDIR;
		}
	}

	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory

	if( strcmp( "/", path ) != 0 )
		parentNodeIndex += g_node_table_block_index; // Convertir a indice absoluto

	for(int currNodeIndex = 0; currNodeIndex < GFILEBYTABLE; currNodeIndex++){
		GFile* currNode = g_node_table + currNodeIndex;
		if( currNode->parent_dir_block == parentNodeIndex && currNode->state != 0 ){
			log_trace( g_logger, "Para %s, encontré: %s", path, currNode->fname );
			filler(buffer, currNode->fname, NULL, 0);
		}
	}

	return 0;
}
*/

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	log_info( g_logger, "[read]:%s", path );
	int currNodeIndex = find_by_path( path );
	if(currNodeIndex == -1){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;

	size_t copySize = min( currNode->file_size - offset, size );
	copy_file_contents(currNode, buffer, copySize, offset, 1);

	return copySize;
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

t_paquete* procesar_mknod ( t_paquete* request ){
	char* path = deserializarMensaje( request->buffer );

	log_info( g_logger, "[mknod]:%s", path );
	int currNode = check_existance_and_availability( path );
	if( currNode < 0 ) {
		free( path );
		return armarPaqueteReturnErrnoConOperacion( currNode, currNode, SAC_mknod );
	}

	int parentNode = get_parent_node( path );
	if( parentNode == -ENOTDIR ){
		free( path );
		return armarPaqueteReturnErrnoConOperacion( -ENOENT, -ENOENT, SAC_mknod );
	}

	log_info( g_logger, "[mknod]:%s ocupa nodo %d", path, currNode );
	occupy_node( path, currNode, parentNode, 1 );

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	free( path );
	return armarPaqueteReturnErrnoConOperacion( 0, 0, SAC_mknod );
}


static int do_mkdir(const char *path, mode_t mode){
	log_info( g_logger, "[mkdir]:%s", path );
	int currNode = check_existance_and_availability( path );
	if( currNode < 0 )
		return currNode;

	int parentNode = get_parent_node( path );
	if( parentNode == -ENOTDIR )
		return -ENOTDIR;

	log_info( g_logger, "[mkdir]:%s ocupa nodo %d", path, currNode );
	occupy_node( path, currNode, parentNode, 2 );

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return 0;
}

static int do_unlink (const char *path){
	log_info( g_logger, "[unlink]:%s", path );
	int currNodeIndex = find_by_path(path);
	if (currNodeIndex == -1)
		return -ENOENT;
	GFile* currNode = g_node_table + currNodeIndex;

	currNode->state = 0;
	liberarBloques(currNode, get_occupied_datablocks_qty( currNode->file_size ), 0 );

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco
	return 0;
}

static int do_rmdir( const char *path ){
	log_info( g_logger, "[rmdir]:%s", path );
	int currNode = find_by_path( path );
	GFile *fileNode = g_node_table + currNode;
	fileNode->state = 0;

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco
	return 0;
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

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi){
	log_info( g_logger, "[write]:%s", path );
	int currNodeIndex = find_by_path( path );
	if(currNodeIndex == -1){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;

	int currUsedBlocks = get_occupied_datablocks_qty(currNode->file_size);
	int neededBlocks = get_occupied_datablocks_qty(size + offset);
	if (size + offset > currNode->file_size) {
		// Lo que quiero escribir mas donde lo quiero escribir se pasa del tamanio actual
		if( neededBlocks > currUsedBlocks ){
			reservarBloques(currNode, currUsedBlocks, neededBlocks);
			// TODO semaforo para bitmap
			/*
			 * TODO preguntar si tengo bloques disponibles
			 * sino return -EFBIG; o -EDQUOT no estoy seguro revisar man 2 write
			 */
			// TODO validar que si quiere reservar mas bloques que no se pase de 1000 * 1024 bloques ( lo maximo por archivo )
		}
		currNode->file_size = size + offset;
	}

	copy_file_contents(currNode, buffer, size, offset, 0);
	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return size;
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

static int do_truncate(const char* path, off_t size){
	log_info( g_logger, "[truncate]:%s a size %ld", path, size );
	int currNodeIndex = find_by_path(path);
	if( currNodeIndex == -1 ){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;

	if( !is_enough_blocks( currNode->file_size, size ) ){
		// errno()
		return -EFBIG;
	}

	int cantidadDatablocksActuales   = get_occupied_datablocks_qty( currNode->file_size );
	int cantidadDatablocksNecesarios = get_occupied_datablocks_qty( size );

	if( cantidadDatablocksActuales > cantidadDatablocksNecesarios )
		liberarBloques(currNode, cantidadDatablocksActuales, cantidadDatablocksNecesarios);
	else if( cantidadDatablocksActuales < cantidadDatablocksNecesarios )
		reservarBloques(currNode, cantidadDatablocksActuales, cantidadDatablocksNecesarios);

	currNode->file_size = size;
	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return 0;
}

t_paquete* procesar_utimens ( t_paquete* request ){
	t_utimens_request* utimens_req = deserializarUtimensReq( request->buffer );
	log_info( g_logger, "[utimens]:%s", utimens_req->path );

	int currNodeIndex = find_by_path(utimens_req->path);
	if( currNodeIndex == -1 ){
		destruirUtimensReq( utimens_req );
		return armarPaqueteReturnErrnoConOperacion( -ENOENT, -ENOENT, SAC_utimens );
	}

	GFile* currNode = g_node_table + currNodeIndex;
	currNode->m_date = utimens_req->modified_time;
	// solo existe last modified time en SAC asi que no tengo fecha de ultimo acceso que actualizar

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	destruirUtimensReq( utimens_req );
	return armarPaqueteReturnErrnoConOperacion( 0, 0, SAC_utimens );
}

long fileSize(char *fname) {
    struct stat stat_buf;
    int rc = stat(fname, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void init_fs(char* disc_path) {
	int fd = open( disc_path, O_RDWR );

	g_disk_size = fileSize( disc_path );
	g_first_block = mmap( NULL, g_disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	g_header = (GHeader*) g_first_block;

	char* name = calloc(4, 1);
	memcpy(name, g_header->sac, 3);
	g_block_count = g_disk_size / GBLOCKSIZE;
	log_info( g_logger, "Nombre: %s", name);
	log_info( g_logger, "Version: %u", g_header->version);
	log_info( g_logger, "Tamanio bitmap %u", g_header->size_bitmap);
	log_info( g_logger, "Tamanio archivo %ld bytes", g_disk_size);
	log_info( g_logger, "Cantidad de bloques %d", g_block_count);

	g_bitmap = bitarray_create_with_mode(((char *) g_first_block)+ g_header->blk_bitmap * GBLOCKSIZE, g_header->size_bitmap * GBLOCKSIZE, MSB_FIRST);
	int occupied = 0, free = 0;
	for( int i  = 0; i < g_block_count; i++){
		if( bitarray_test_bit(g_bitmap, i)){
			occupied ++;
		} else {
			free ++ ;
		}
	}
	log_debug( g_logger, "%d libres con %d ocupados", free, occupied );

	g_node_table_block_index = g_header->blk_bitmap + g_header->size_bitmap;
	g_node_table = (GFile*) g_header + g_node_table_block_index;
}