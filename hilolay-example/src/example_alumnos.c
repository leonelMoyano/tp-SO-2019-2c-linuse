#include <hilolay/alumnos.h>
#include <hilolay/internal.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* Lib implementation: It'll only schedule the last thread that was created */
int max_tid = 0;

int suse_create(int tid){
	if (tid > max_tid) max_tid = tid;
	return 0;
}

int suse_schedule_next(void){
	int next = max_tid;
	printf("Scheduling next item %i...\n", next);
	return next;
}

int suse_join(int tid){
	// Not supported
	return 0;
}

int suse_close(int tid){
	printf("Closed thread %i\n", tid);
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

void hilolay_init(void){
	printf("\n\nMacri gato\n\n");
	init_internal(&operaciones);
}

