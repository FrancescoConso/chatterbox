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
 * @file threadpool.c
 * @brief Modulo threadpool del server
 * 
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

//#include "message.h"
//#include "connections.h"
#include <threadpool.h>
#include <handlerClient.h>


/** @function threadpoolThreadAvvia
 * 	
 * @brief il metodo che viene lanciato alla creazione del thread worker della pool, si occupa dell'handling dei messaggi
 * 
 * @param threadpool	la threadpool a cui appartiene il thread worker
 * 
 * @return NULL
 */

void * threadpoolThreadAvvia(void * threadpool)
{
	Threadpool * riferimentoThreadpool = (Threadpool *) threadpool;
	ThreadpoolTask task;
	
	while(riferimentoThreadpool -> flagTerminazione == 0)
	{
		//acquisisco lock
		pthread_mutex_lock(&(riferimentoThreadpool -> lockThread));
																		//printf ("Acquisisco la mutex \n");
		
		//mi metto in attesa all'interno di un while, per evitare wakeup spuri del thread worker
		while((riferimentoThreadpool -> contatorePendenti == 0) && (riferimentoThreadpool -> flagTerminazione == 0))
		{
			pthread_cond_wait(&(riferimentoThreadpool -> svegliaThread), &(riferimentoThreadpool -> lockThread)); //printf ("Mi metto in attesa sulla condizione \n");													
		}
		
		//se fossi in fase di flagTerminazione
		if((riferimentoThreadpool -> flagTerminazione == 1) && (riferimentoThreadpool -> contatorePendenti == 0))
		{
			break;
		}
		
		//recupero il task dalla coda pendenti
		NodoCodaPendenti * nodoSuccessivo = NULL;
		nodoSuccessivo = riferimentoThreadpool -> codaPendenti -> next;
		task . descrittoreSocket = riferimentoThreadpool -> codaPendenti -> task . descrittoreSocket;
		free(riferimentoThreadpool -> codaPendenti);
		riferimentoThreadpool -> codaPendenti = nodoSuccessivo;

		riferimentoThreadpool -> contatorePendenti--;
		
		//sblocca mutex
		pthread_mutex_unlock(&(riferimentoThreadpool -> lockThread));
		
		//lavora - codice nell'handler
																		//printf("Lavora descrittore %d\n",(int)  task . descrittoreSocket);
		gestioneClient(task . descrittoreSocket);
	}
	riferimentoThreadpool -> contatoreAvviati--;
	
	pthread_mutex_unlock(&(riferimentoThreadpool -> lockThread));

	//printf("LA MIA VITA DA THREAD E' FINITA\n");
	return (NULL);
}

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

int threadpoolTermina(Threadpool * threadpool)
{
																		//printf ("Avvio flagTerminazione threadpool \n");

	if(threadpool == NULL) {fprintf(stderr, "Errore di accesso alla threadpool per la Terminazione\n"); return 0;}
	if(pthread_mutex_lock(&(threadpool -> lockThread)) != 0) {fprintf(stderr, "Errore di blocco della pool per la flagTerminazione\n"); return 0;}
	
	//controllo se fosse già in flagTerminazione 
	if(threadpool -> flagTerminazione == 1) {fprintf(stderr, "Siamo già in Terminazione"); return 0;}
	//se no posso avviare tranquillamente
	threadpool -> flagTerminazione = 1;
	
																		//printf ("Svegliamo thread worker \n");
	
	//sveglio tutti i thread worker se != 0 errore
	if(pthread_cond_broadcast(&(threadpool -> svegliaThread)) != 0) {fprintf(stderr, "Errore di broadcast ai thread della pool per la flagTerminazione\n"); return 0;}
	if(pthread_mutex_unlock(&(threadpool -> lockThread)) != 0) {fprintf(stderr, "Errore di sblocco della lock della pool per la flagTerminazione\n"); return 0;}

	
	
	//attendo che i worker terminino
	for(int i=0; i<threadpool -> dimensionePool; i++)
	{
		
		if(pthread_join(threadpool -> insiemeThread[i], NULL) != 0) {fprintf(stderr, "Errore di attesa di uno dei thread\n"); return 0;}
		

																		printf ("Joinato %d thread \n",i);
		
	}
	
	//dealloco l'insieme dei thread							//CRITICO
	//for(int i=0; i<threadpool -> dimensionePool; i++)		//CRITICO
	//	free((void *) threadpool -> insiemeThread[i]);		//CRITICO
	
	
	
	//se è andato tutto bene dealloco la pool
	if(threadpool -> codaPendenti != NULL) free(threadpool -> codaPendenti);
	
	if(threadpool -> insiemeThread != NULL) 
	{
		free(threadpool -> insiemeThread);
																		//printf ("Liberata lista thread \n");
		
																		//printf ("Liberata coda pendenti \n");
		//blocco la mutex per sicurezza
																		//printf ("mutex bloccata \n");
		pthread_mutex_lock(&(threadpool -> lockThread));
																		//printf ("mutex distrutta \n");
		pthread_mutex_destroy(&(threadpool -> lockThread));
																		//printf ("cond distrutta \n");
		pthread_cond_destroy(&(threadpool -> svegliaThread));

	}
	free(threadpool);
	
	return 1;
}

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

Threadpool * threadpoolInit(int dimensionePool)
{

	if((dimensionePool <= 0)) {fprintf(stderr, "Numero di thread nullo\n"); return NULL;}
	
	Threadpool * nuovaThreadpool;
	if((nuovaThreadpool = (Threadpool *) malloc(sizeof (Threadpool))) == NULL) {fprintf(stderr, "Errore nell'allocazione della threadpool\n"); return NULL;}
	
	//inizializzo i parametri
	nuovaThreadpool -> dimensionePool = 0;
	nuovaThreadpool -> contatoreAvviati=0;
	nuovaThreadpool -> contatorePendenti=0;
	nuovaThreadpool -> flagTerminazione=0;
	
	
	//alloco i thread e la lista pendenti
	if((nuovaThreadpool -> insiemeThread = (pthread_t *) malloc(sizeof(pthread_t)*dimensionePool)) == NULL) {fprintf(stderr, "Errore nell'allocazione della pool di thread\n"); return NULL;}
	nuovaThreadpool -> codaPendenti = NULL;
	
	if(pthread_mutex_init(&(nuovaThreadpool -> lockThread), NULL) != 0) {fprintf(stderr, "Errore nella inizializzazione della lock thread\n"); return NULL;}
	if(pthread_cond_init(&(nuovaThreadpool -> svegliaThread), NULL) != 0) {fprintf(stderr, "Errore nell'allocazione della sveglia thread\n"); return NULL;}
	
	//inizio dei thread worker
	for(int i=0; i<dimensionePool; i++)
	{
																		//printf("creato pthread %d\n", i);
		//il metodo threadpoolThreadAvvia contiene la routine di avviamento del thread
		if(pthread_create(&(nuovaThreadpool -> insiemeThread[i]), NULL, threadpoolThreadAvvia, (void *)nuovaThreadpool) != 0) {fprintf(stderr, "Errore nell'avvio dei thread worker\n"); 
																															return NULL;}
																																																			
		nuovaThreadpool -> dimensionePool++;
		nuovaThreadpool -> contatoreAvviati++;
	}
	
	return nuovaThreadpool;
}

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

int threadpoolCodaPendentiIncodaTask (Threadpool * threadpool, int descrittoreSocket)
{
	if(threadpool == NULL) {fprintf(stderr, "Errore nell'accesso alla threadpool\n"); return 0;}
	
	//se fosse in terminazione
	if((threadpool -> flagTerminazione == 1) && (threadpool -> contatorePendenti == 0)) return 0;
	
	if(pthread_mutex_lock(&(threadpool -> lockThread)) != 0) {fprintf(stderr, "Errore nell'acquisizione della mutua esclusione\n"); return 0;}
	
	//alloco il thread
	NodoCodaPendenti * nuovoTask = malloc(sizeof(NodoCodaPendenti));
	if(nuovoTask == NULL) {fprintf(stderr, "Errore nell'allocazione del nodo\n"); return 0;}
	
	nuovoTask -> task . descrittoreSocket = descrittoreSocket;
	nuovoTask -> next = NULL;
	
	//possiamo aggiungere il thread in coda iterando la lista concatenata e appendendo
	
	NodoCodaPendenti * iteratore = threadpool -> codaPendenti;
	//Se fosse il primo lo aggiungo alla coda, altrimenti itero fino a raggiungere la coda
	if (iteratore == NULL)	{threadpool -> codaPendenti = nuovoTask;}
	else {
	while (iteratore -> next != NULL)	iteratore = iteratore -> next;
	iteratore -> next = nuovoTask;}
	
	threadpool -> contatorePendenti ++;
	
	//segnalo che è presente un thread
	if(pthread_cond_signal(&(threadpool -> svegliaThread)) != 0) {fprintf(stderr, "Errore nella segnalazione ai thread\n"); return 0;}
	
	//sblocca mutex
	if(pthread_mutex_unlock(&(threadpool -> lockThread)) != 0) {fprintf(stderr, "Errore nello sblocco della mutua esclusione\n"); return 0;}
	
	return 1;

	
}
