/*
 * Progetto Corso Sistemi Operativi e Laboratorio 2018
 * 
 * Francesco Consonni
 * francescoconsoconsonni@gmail.com
 * Matricola # 531373
 * 
 * Si dichiara che il programma è opera originale dello studente.
 * 
 */

/**
 * Il modulo threadpool del server gestisce la threadpool del server verso la quale il thread listener
 * incoda il file descriptor della socket da cui leggere i dati fino a disconnessione.
 * 
 * @file threadpool.h
 * @brief Header del modulo threadpool del server
 * 
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

//#include "message.h"
//#include "connections.h"


typedef struct ThreadpoolTask {
	long descrittoreSocket; //descrittore del messaggio
	} ThreadpoolTask;

typedef struct NodoCodaPendenti {
	ThreadpoolTask task;			//task del nodo della coda
	struct NodoCodaPendenti * next;	//nodo successivo
} NodoCodaPendenti;

typedef struct Threadpool {		
	pthread_mutex_t lockThread;											//Lock di mutua esclusione sulla threadpool
	pthread_cond_t svegliaThread;										//Variabile di condizione sulla threadpool
	pthread_t * insiemeThread;											//Il numero di thread della pool
	
	NodoCodaPendenti * codaPendenti;									//Lista che rappresenta la coda dei task pendenti
	int contatorePendenti;												//Contatore dei task pendenti in un certo istante
	
	int contatoreAvviati;												//Contatore dei thread in esecuzione in questo momento
	int dimensionePool;													//Dimensione della threadpool

	int flagTerminazione;												//Segnala che i thread sono in terminazione

	} Threadpool;


/** @function threadpoolTermina
 * 	
 * @brief termina e dealloca la threadpool
 * 
 * @param threadpool	la threadpool da terminare e deallocare
 * 
 * @return	1 se l'operazione ha avuto successo
 * 			0 altrimenti
 * 
 */

int threadpoolTermina(Threadpool * threadpool);

/** @function threadpoolInit
 * 	
 * @brief inizializza la threadpool
 * 
 * @param dimensionePool	numero di thread nella pool
 * 
 * @return	puntatore alla threadpool se l'operazione ha avuto successo
 * 			NULL altrimenti
 * 
 */

Threadpool * threadpoolInit(int dimensionePool);

/** @function threadpoolCodaPendentiIncodaTask
 * 	
 * @brief aggiunge un nuovo task alla coda pendenti
 * 
 * @param threadpool			la threadpool su cui inserire il messaggio
 * @param descrittoreSocket	il descrittore del messaggio da inserire
 * 
 * @return	1 se l'operazione ha avuto successo
 * 			0 altrimenti
 * 
 */

int threadpoolCodaPendentiIncodaTask (Threadpool * threadpool, int descrittoreSocket);

#endif
