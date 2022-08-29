//Modulo connectionLocks.c
//Contiene un sistema di locks che gestisce la scrittura sulle socket

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <connectionSocketLocks.h>

//la select quando un client si connette restituisce un fd, questo viene
//inoltrato alla threadpool, un thread handler la prende e la gestisce

//thread devono avere un riferimento ad un array di lock in comune, quello delle socket

/*
 * Variabili globali
 * numeroSocketLocks 			 - numero delle lock sull'array
 */
static int numeroSocketLocks;

//static pthread_mutex_t * socketLocks; //per consentire un certo livello di granularità le lock sono gestite come array

/** @function connectionSocketLocksInit
 * 
 * @brief inizializza le socket lock
 * 
 * @param	numeroSocketLocksInit			numero di lock
 * 
 * @return	socketLocks se ha avuto successo
 * 			NULL altrimenti
 * 
 */

pthread_mutex_t * connectionSocketLocksInit (int numeroSocketLocksInit)
{
	//costruisco le lock
	numeroSocketLocks = numeroSocketLocksInit;
	pthread_mutex_t * socketLocks = NULL;
	socketLocks = malloc(sizeof(pthread_mutex_t)*numeroSocketLocks);
	if(socketLocks == NULL) {fprintf(stderr, "Errore nell'allocazione delle socket lock \n"); return NULL;}
	
	//attivo le lock
	for(int i=0; i<numeroSocketLocks; i++)
	if(pthread_mutex_init(&socketLocks[i], NULL) != 0) {fprintf(stderr, "Errore nell'inizializzazione socket lock\n"); return NULL;}
	
	return socketLocks;
}

/** @function connectionSocketLocksTermina
 * 
 * @brief termina e dealloca le socket lock
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */
 
int connectionSocketLocksTermina (pthread_mutex_t * socketLocks)	{

	//distruggo le lock
	for(int i=0; i<numeroSocketLocks; i++)	{
		//if(pthread_mutex_lock(&socketLocks[i]) != 0) {fprintf(stderr, "Errore nell'acquisizione della socket lock \n"); return 0;}			
		if(pthread_mutex_destroy(&socketLocks[i]) != 0) {fprintf(stderr, "Errore nella distruzione della socket lock\n"); return 0;}			
	}
	free(socketLocks);
	
	return 1;
}

/** @function connectionSocketLocksLock
 * 
 * @brief acquisisce una socketLock
 * 
 * @param	fd			file descriptor
 * @param	socketLocks			array delle lock
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int connectionSocketLocksLock (int fd, pthread_mutex_t * socketLocks)
{
	//mi impossesso della lock
	unsigned int numeroLock = fd % numeroSocketLocks;
	if(pthread_mutex_lock(&socketLocks[numeroLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della socket lock\n"); return 0;}		
	else return 1;
}

/** @function connectionSocketLocksUnlock
 * 
 * @brief rilascia una socketLock
 * 
 * @param	fd			file descriptor
 * @param	socketLocks			array delle lock
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int connectionSocketLocksUnlock (int fd, pthread_mutex_t * socketLocks)
{
	//libero della lock
	unsigned int numeroLock = fd % numeroSocketLocks;
	if(pthread_mutex_unlock(&socketLocks[numeroLock]) != 0) {fprintf(stderr, "Errore nella liberazione della socket lock\n"); return 0;}		
	else return 1;
}
