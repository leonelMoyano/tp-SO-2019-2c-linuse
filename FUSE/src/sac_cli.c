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
#include "biblioNOC/paquetes.h"
#include "sac.h"
#include "fuse_utils.h"

ptrGBloque* g_first_block; // Puntero al inicio del primer bloque dentro del archivo
long g_disk_size; // Tamanio en bytes del archivo
GHeader* g_header; // Puntero al header del FS
GFile* g_node_table; // Puntero al primer nodo del FS
u_int32_t g_block_count; // Cantidad de bloques en el archivo
t_bitarray* g_bitmap; // Puntero al bitmap del FS

/*
 * Esta es una estructura auxiliar utilizada para almacenar parametros
 * que nosotros le pasemos por linea de comando a la funcion principal
 * de FUSE
 */
struct t_runtime_options {
	char* disk;
} runtime_options;

/*
 * Esta Macro sirve para definir nuestros propios parametros que queremos que
 * FUSE interprete. Esta va a ser utilizada mas abajo para completar el campos
 * welcome_msg de la variable runtime_options
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


/**
* @NAME: find_by_name
* @DESC: Devuelve el indice en la tabla de nodos para el archivo, -1 en caso de no encontrarlo
*
*/
int find_by_name(const char *path){
	// Por el momento asumo que todx esta en / asi que aca solo recorro la tabla de nodos y pregunto por el nombre
	GFile* currNode;
	for(int currNodeIndex = 0; currNodeIndex < GFILEBYTABLE; currNodeIndex++){
		currNode = g_node_table + currNodeIndex;
		if( strcmp( path + 1, currNode->fname ) == 0 && currNode->state != 0){ // path + 1 para omitir la barra
			return currNodeIndex;
		}
	}
	return -1;
}

static int do_getattr(const char *path, struct stat *st) {
	printf("[getattr] Called\n");
	printf("\tAttributes of %s requested\n", path);

	// GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
	// 		st_uid: 	The user ID of the file’s owner.
	//		st_gid: 	The group ID of the file.
	//		st_atime: 	This is the last access time for the file.
	//		st_mtime: 	This is the time of the last modification to the contents of the file.
	//		st_mode: 	Specifies the mode of the file. This includes file type information (see Testing File Type) and the file permission bits (see Permission Bits).
	//		st_nlink: 	The number of hard links to the file. This count keeps track of how many directories have entries for this file. If the count is ever decremented to zero, then the file itself is discarded as soon
	//						as no process still holds it open. Symbolic links are not counted in the total.
	//		st_size:	This specifies the size of a regular file in bytes. For files that are really devices this field isn’t usually meaningful. For symbolic links this specifies the length of the file name the link refers to.

	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now

	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now
		return 0;
	}

	// path aca es por ej "/testFoo"
	int currNodeIndex = find_by_name(path);
	if (currNodeIndex != -1) {
		GFile* currNode = g_node_table + currNodeIndex;
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_mtime = currNode->m_date;
		st->st_size = currNode->file_size;
		return 0;
	}

	return -ENOENT;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	printf("--> Getting The List of Files of %s\n", path);

	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory

	int currNodeIndex = 0;
	for(; currNodeIndex < GFILEBYTABLE; currNodeIndex++){ // Por el momento asumo que todx existe en /
		GFile* currNode = g_node_table + currNodeIndex;
		if( currNode->state == 1 ){
			// TODO logear algo aca ?
			filler(buffer, currNode->fname, NULL, 0);
		}
	}

	/*
	 * TODO cuando tenga una estructura de directorios y no todx tirado en /
	 * voy a tener una llamada a una funcion que dado un path devuelve el id de directorio
	 * cuando tenga ese id voy a hacer el for buscando todx lo que tenga estado != 0
	 * y con el parent node en ese id
	 */

	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf( "[read]: %s\n", path);
	int currNodeIndex = find_by_name( path );
	if(currNodeIndex == -1){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;

	// TODO aca el size puede venir con 4096 por ej aun cuando getattr da filesize en 0
	copy_file_contents(currNode, buffer, size, offset, 1);

	return size;
}

static int do_mknod (const char *path, mode_t mode, dev_t device){
	int currNode = find_by_name(path);
	if( currNode != -1 ){
		return -EEXIST; // path already exists
	}
	currNode = 0;

	while( g_node_table[currNode].state!=0 && currNode < GFILEBYTABLE ) // Busco la proxima entrada disponible en la tabla de nodos
		currNode++;

	if (currNode >= g_block_count)
		return -EDQUOT; // No tengo nodos disponibles para crear otro archivo

	GFile* nodeToSet = g_node_table + currNode;
	// TODO pasar esto a log
	printf( "[mknod][%d]: %s\n", currNode, path);
	strcpy((char*) nodeToSet->fname, path + 1); // TODO IMPORTATEN + 1 ES PARA SALTEARSE LA BARRA
	nodeToSet->state = 1;
	nodeToSet->file_size = 0;
	time_t now = time( NULL);
	printf("sizeof time_t %u", sizeof(now));
	nodeToSet->c_date = now;

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return 0;
}

static int do_unlink (const char *path){
	int currNodeIndex = find_by_name(path);
	if (currNodeIndex != -1) {
		GFile* currNode = g_node_table + currNodeIndex;
		currNode->state = 0;

		msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco
		return 0;
	}

	return -ENOENT;
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

		if(read_mode == 1)
			memcpy((char *)(buffer + size - pendingSize), (void*)filePointer, copySize);
		else
			memcpy((void*)filePointer, buffer + size - pendingSize, copySize);

		spaceLeftOnBlock = GBLOCKSIZE; // porque el proximo bloque lo voy a tener desde el principio
		pendingSize -= copySize;
		// muevo el filePointer a principio de proximo bloque ( offset 4096 es el byte 0 del block 1 )
		// tal vez esto deberia estar dentro de un if(pendingSize) por si estaba apuntando al ultimo bloque ( 1000 * 1024 ) tal vez tenga comportamiento raro
		filePointer = seek_offset(fileNode, dataBlockIndex * GBLOCKSIZE);
		dataBlockIndex++;
	}
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi){
	printf( "[write]: %s\n", path);
	int currNodeIndex = find_by_name( path );
	if(currNodeIndex == -1){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;

	int currUsedBlocks = currNode->file_size / GBLOCKSIZE;
	int neededBlocks = (size + offset) / GBLOCKSIZE;
	if (size + offset > currNode->file_size || currNode->file_size == 0) {
		// Lo que quiero escribir mas donde lo quiero escribir se pasa del tamanio actual
		if( currUsedBlocks > neededBlocks ){
			// Los bloques que ya tengo reservados no alcanzan
			int bloquesFaltantes = neededBlocks - currUsedBlocks;

			int nuevoDatablock;
			int nuevoIndirect;
			int proxDatablock = currNode->file_size == 0 ? 0 : currUsedBlocks % 1024 + 1;
			int proxIndirect = currNode->file_size == 0 ? 0 : currUsedBlocks / 1024 + 1;

			GPtrIndSimple * indirectBlock;
			// tal vez valga la pena reescribir esto para no tener el proximo indice sino el actual y que sea menos confuso

			for (int i = 0; i < bloquesFaltantes; i++) {
				if( proxDatablock % 1024 == 0 ){
					// necesito un bloque indirecto
					nuevoIndirect = get_avail_block();
					currNode->blk_indirect[proxIndirect] = nuevoIndirect;
					bitarray_set_bit(g_bitmap, nuevoIndirect);
					proxIndirect++;
					proxDatablock = 0;
				}
				nuevoDatablock = get_avail_block();
				indirectBlock = (GPtrIndSimple*) g_first_block + currNode->blk_indirect[proxIndirect - 1];
				indirectBlock->blk_direct[ proxDatablock ] = nuevoDatablock;
				bitarray_set_bit(g_bitmap, nuevoDatablock);
				nuevoDatablock++;
			}
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

	return size;

}

static int do_utimens( const char *path, const struct timespec tv[2]){
	// donde tv[ 0 ] es last access time y tv[ 1 ] es last modified time
	int currNodeIndex = find_by_name(path);
	if( currNodeIndex == -1 ){
		return -ENOENT;
	}

	GFile* currNode = g_node_table + currNodeIndex;
	currNode->m_date = tv[0].tv_sec;
	// solo existe last modified time en SAC asi que no tengo fecha de ultimo acceso que actualizar

	msync( g_first_block, g_disk_size, MS_SYNC ); // Para que lleve los cambios del archivo a disco

	return 0;
}

static struct fuse_operations operations = {
		.getattr = do_getattr,
		.readdir = do_readdir,
		.read = do_read,
		.mknod = do_mknod,
		.unlink = do_unlink,
		.utimens = do_utimens,
		.write = do_write,
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
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--disk %s", disk, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

long fileSize(char *fname) {
    struct stat stat_buf;
    int rc = stat(fname, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char *argv[]) {
	// TODO init logger
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	if( runtime_options.disk != NULL ){
		printf("Mounting disk path: %s\n", runtime_options.disk);
	} else {
		perror("falta parametro --disk %s");
		exit(1);
	}

	int fd = open( (char*)runtime_options.disk, O_RDWR );

	g_disk_size = fileSize( (char*)runtime_options.disk );
	g_first_block = mmap( NULL, g_disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	g_header = (GHeader*) g_first_block;

	char* name = calloc(4, 1);
	memcpy(name, g_header->sac, 3);
	g_block_count = g_disk_size / GBLOCKSIZE;
	// TODO pasar esto a logs
	printf("Nombre: %s\n", name);
	printf("Version: %u\n", g_header->version);
	printf("Tamanio bitmap %u\n", g_header->size_bitmap);
	printf("Tamanio archivo %ld bytes\n", g_disk_size);
	printf("Cantidad de bloques %d\n", g_block_count);

	g_bitmap = bitarray_create_with_mode(((char *) g_first_block)+ g_header->blk_bitmap * GBLOCKSIZE, g_header->size_bitmap * GBLOCKSIZE, MSB_FIRST);
	int occupied = 0, free = 0;
	for( int i  = 0; i < g_block_count; i++){
		if( bitarray_test_bit(g_bitmap, i)){
			occupied ++;
		} else {
			free ++ ;
		}
	}
	printf( "%d libres con %d ocupados", free, occupied);

	g_node_table = (GFile*) g_header + g_header->blk_bitmap + g_header->size_bitmap;

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todx
	// en varios threads
	return fuse_main(args.argc, args.argv, &operations, NULL);
}
