/*
 * biblioSuse.h
 *
 *  Created on: 31 oct. 2019
 *      Author: utnso
 */

#ifndef BIBLIOSUSE_H_
#define BIBLIOSUSE_H_


int suse_create(int tid);
int suse_schedule_next(void);
int suse_join(int tid);
int suse_close(int tid);
int suse_wait(int tid, char *sem_name);
int suse_signal(int tid, char *sem_name);

struct hilolay_operations operaciones;

void hilolay_init(void);

#endif /* BIBLIOSUSE_H_ */
