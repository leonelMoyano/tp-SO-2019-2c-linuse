/*
 * fuse_utils.h
 *
 *  Created on: 14 nov. 2019
 *      Author: utnso
 */

#ifndef FUSE_UTILS_H_
#define FUSE_UTILS_H_

int get_datablock_index(off_t);
int get_indirect_block_index(off_t);
int get_datablock_index_inside_indirect_block(off_t);

int find_by_name(const char*);

size_t min(size_t, size_t);

void copy_file_contents(GFile*, const char*, size_t, off_t, int);

#endif /* FUSE_UTILS_H_ */
