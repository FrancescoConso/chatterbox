//Modulo connectionLocks.c
//Contiene un sistema di locks che gestisce la scrittura sulle socket

#ifndef CONNECTIONSOCKETLOCKS_H
#define CONNECTIONSOCKETLOCKS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>



//la select quando un client si connette restituisce un fd, questo viene
//inoltrato alla threadpool, un thread handler la prende e la gestisce

//thread devono avere un riferimento ad un array di lock in comune, quello delle socket



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

pthread_mutex_t * connectionSocketLocksInit (int numeroSocketLocksInit);

/** @function connectionSocketLocksTermina
 * 
 * @brief termina e dealloca le socket lock
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */
 
int connectionSocketLocksTermina (pthread_mutex_t * socketLocks);

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

int connectionSocketLocksLock (int fd, pthread_mutex_t * socketLocks);

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

int connectionSocketLocksUnlock (int fd, pthread_mutex_t * socketLocks);

#endif
