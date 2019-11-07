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
#include <stdlib.h>
#include <stdio.h>
#include <commons/temporal.h>
#include <commons/log.h>
#include "biblioNOC/paquetes.h"
#include "sac.h"

ptrGBloque* g_first_block;
long g_disk_size;
GHeader* g_header;
GFile* g_node_table;
u_int32_t g_node_count;


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



// TODO empece a seguir este tuto http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
// TODO tambien sacar cosas de aca https://github.com/sisoputnfrba/so-fuse_example/


/**
* @NAME: find_by_name
* @DESC: Devuelve el indice en la tabla de nodos para el archivo, -1 en caso de no encontrarlo
*
*/
int find_by_name( const char *path){
	// Por el momento asumo que todx esta en / asi que aca solo recorro la tabla de nodos y pregunto por el nombre
	GFile* currNode;
	for(int currNodeIndex = 0; currNodeIndex < GFILEBYTABLE; currNodeIndex++){
		currNode = g_node_table + currNodeIndex;
		if( strcmp( path + 1, currNode->fname ) == 0 ){ // path + 1 para omitir la barra
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
	st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}

	// path aca es por ej "/testFoo"
	int currNodeIndex = find_by_name(path);
	if (currNodeIndex != -1) {
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
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
			filler(buffer, currNode->fname, NULL, 0); // TODO falta probar esto
		}
	}

	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	char fileText[] = "Hello World From any file( always returning the same for testing purposes )!\n";

	printf( "[read]: %s\n", path);

	memcpy(buffer, fileText + offset, size);

	return strlen(fileText) - offset;
}

static int do_mknod (const char *path, mode_t mode, dev_t device){
	int currNode = 0;
	while( g_node_table[currNode].state!=0 && currNode < g_node_count ) // Busco la proxima entrada disponible en la tabla de nodos
		currNode++;

	if (currNode >= g_node_count)
		return EDQUOT; // No tengo nodos disponibles para crear otro archivo

	GFile* nodeToSet = g_node_table + currNode;
	// TODO pasar esto a log
	printf( "[mknod][%d]: %s\n", currNode, path);
	strcpy((char*) nodeToSet->fname, path + 1); // TODO IMPORTATEN + 1 ES PARA SALTEARSE LA BARRA
	nodeToSet->state = 1;
	nodeToSet->file_size = 0;

	msync( g_first_block, g_disk_size, MS_SYNC );

	return 0;
}

static struct fuse_operations operations = {
		.getattr = do_getattr,
		.readdir = do_readdir,
		.read = do_read,
		.mknod= do_mknod,
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
	g_node_count = g_disk_size / GBLOCKSIZE;
	// TODO pasar esto a logs
	printf("Nombre: %s\n", name);
	printf("Version: %u\n", g_header->version);
	printf("Tamanio bitmap %u\n", g_header->size_bitmap);
	printf("Tamanio archivo %ld bytes\n", g_disk_size);
	printf("Cantidad de bloques %d\n", g_node_count);


	g_node_table = (GFile*) g_header + g_header->blk_bitmap + g_header->size_bitmap;

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comuniscarse con el kernel, delegar todx
	// en varios threads
	return fuse_main(args.argc, args.argv, &operations, NULL);
}
