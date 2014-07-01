/*
 ============================================================================
 Name        : Zin_tot_RS485receivers.c
 Author      : Luca Buccolini
 Version     :
 Copyright   : Your copyright notice
 Description : Calcola il valore della Zin (impedenza parallelo) di receiver_nr ricevitori RS485 collegati al bus con "Carico Unitario" o "Unit Load" pari a UL.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#define receiver_nr 256
#define UL (1.0/8.0)


int main(void) {
	int i;
	const float zin=12000/(float)UL;
	//calcolo l'impedenza parallello di tutti i ricevitori RS485 con UL specificata in define (1, 1/2, 1/4, 1/8)

	printf("zin singolo ricevitore=%f\n",zin);
	printf("N° di ricevitori collegati al bus=%d\n", receiver_nr);

	float yin_tot=0.0;
	for (i=0; i< receiver_nr; i++) {
		yin_tot+=1.0/zin;
	}
	float zin_tot=1.0/(yin_tot);
	printf("zin_tot=%f\n",zin_tot);
	return EXIT_SUCCESS;
}
