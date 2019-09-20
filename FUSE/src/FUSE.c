#include <stdlib.h>
#include <stdio.h>
#include <commons/temporal.h>
#include "biblioNOC/paquetes.h"

int main(void) {
	int j = 3333;
	int i = prueba();
	while (j >= 3000) {
		char* tiempo = temporal_get_string_time();
		printf("\ncuenta regresiva : %d", j);
		puts (tiempo);
		free(tiempo);
		j--;
	}
	return i;
}
