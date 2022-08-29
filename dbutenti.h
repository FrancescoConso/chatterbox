/*
 * Progetto Corso Sistemi Operativi e Laboratorio 2018
 * 
 * Francesco Consonni
 * francescoconsoconsonni@gmail.com
 * Matricola # 531373
 * 
 * Si dichiara che il programma è opera originale dello studente.
 * 
 * 
 */
#ifndef DBUTENTI_H_
#define DBUTENTI_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
/*
 * il sorgente C di icl_hash è stato parzialmente modificato
 * per correggere un bug nella rimozione di un nodo dalla tabella
 * e per rendere visibile hash_pjw per il calcolo del bucket
 */

#include "icl_hash.h"
#include "message.h"



/* Strutture dati:
 * ListaHistory		 - Array circolare che gestisce la history degli utenti
 * DBUtenti			 - Struttura globale che include tabella hash e lock
 */

typedef struct ListaHistory
{
	int testa;				//si comincia a scaricare i messaggi da qui
	int coda;				//la posizione dell'ultimo messaggio scritto
	int dimensione;			//dimensione parziale della history
	
	message_t * messaggi;
	
} ListaHistory;

typedef struct DBUtenti
{
	icl_hash_t * hash;		//tabella hash strutturata con key = nickname, data = history
	pthread_mutex_t * lock;	//per consentire un certo livello di granularità le lock sono gestite come array
} DBUtenti;

/** @function dbUtentiHashLiberaNickname
 * 
 * @brief libera l'area di memoria che contiene il nickname
 * 
 * @param nickname	l'indirizzo del nickname da liberare
 * 
 */

void dbUtentiHashLiberaNickname(void * nickname);


/** @function dbUtentiHashLiberaHistory
 * 
 * @brief libera l'area di memoria che contiene la history dell'utente
 * 
 * @param history	l'indirizzo della regione che contiene la history
 * 
 */

void dbUtentiHashLiberaHistory(void * history);


/** @function dbUtentiInit
 * 
 * @brief inizializza il database Utenti
 * 
 * @param	numeroBuckets			numero di bucket nella tabella hash
 * @param	dimensioneHistoryInit	dimensione massima della history di un utente
 * @param	numeroLockInit			numero di lock sulla tabella hash
 * 
 * @return	DBUtenti se ha avuto successo
 * 			NULL altrimenti
 * 
 */

DBUtenti * dbUtentiInit(int numeroBuckets, int dimensioneHistoryInit, int numeroLockInit);


/** @function dbUtentiRegistraNickname
 * 
 * @brief registra un nickname nel dbUtenti
 * 
 * @param database		il database in cui registrare il nickname
 * @param nickname		il nickname da registrare
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiRegistraNickname(DBUtenti * database, char * nickname);	


/** @function dbUtentiDeregistraNickname
 * 
 * @brief deregistra un nickname nel dbUtenti
 * 
 * @param database		il database in cui deregistrare il nickname
 * @param nickname		il nickname da deregistrare
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiDeregistraNickname(DBUtenti * database, char * nickname);

/** @function dbUtentiCercaNickname
 * 
 * @brief cerca un nickname nel dbUtenti
 * 
 * @param database		il database in cui deregistrare il nickname
 * @param nickname		il nickname da deregistrare
 * 
 * @return	1 se il nickname esiste
 * 			0 altrimenti
 * 
 */

int dbUtentiCercaNickname(DBUtenti * database, char * nickname);

/** @function dbUtentiAggiungiHistory
 * 
 * @brief aggiunge un messaggio a una history
 * 
 * @param historyUtente		la history a cui aggiungere il messaggio
 * @param messaggio			il messaggio da inserire nella History
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiAggiungiHistory (ListaHistory * historyUtente, message_t * messaggio);	//ritorna 1 se ha successo, 0 altrimenti


/** @function dbUtentiRitrovaHistory
 * 
 * @brief restituisce un puntatore alla history di un utente
 * 
 * @param database		database su cui cercare la history
 * @param nickname		nickname dell'utente di cui cercare la history
 * 
 * @return	ListaHistory se ha avuto successo
 * 			NULL altrimenti
 * 
 */

ListaHistory * dbUtentiRitrovaHistory(DBUtenti * database, char * nickname);	//ritorna la lista history se ha successo NULL altrimenti


/** @function dbUtentiCopiaHistory
 * 
 * @brief restituisce una copia della history di un utente
 * 
 * @param database		database su cui cercare la history
 * @param nickname		nickname dell'utente di cui cercare la history
 * 
 * @return	ListaHistory se ha avuto successo
 * 			NULL altrimenti
 * 
 */

ListaHistory * dbUtentiCopiaHistory(DBUtenti * database, char * nickname);	//facciamo una copia per evitare di occupare la lock durante l'invio


/** @function dbUtentiInviaMessaggio
 * 
 * @brief aggiunge alla history di un utente un messaggio
 * 
 * @param database		database su cui cercare la history
 * @param op			tipo di operazione del messaggio
 * @param sender		mittente del messaggio
 * @param receiver		destinatario del messaggio
 * @param buf			testo del messaggio
 * @param len			lunghezza del testo
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiInviaMessaggio(DBUtenti * database, op_t op, char * sender, char * receiver, char * buf, unsigned int len );	//1 se ha successo, 0 altrimenti


/** @function dbUtentiBroadcastMessaggio
 * 
 * @brief aggiunge alla history di tutti gli utenti meno il mittente un messaggio
 * 
 * @param database		database su cui cercare la history
 * @param op			tipo operazione
 * @param sender		mittente del messaggio
 * @param buf			testo del messaggio
 * @param len			lunghezza del testo
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiBroadcastMessaggio (DBUtenti * database, op_t op, char * sender, char * buf, unsigned int len); //il broadcast è l'operazione 3


/** @function dbUtentiTermina
 * 
 * @brief termina e dealloca il dbutenti
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiTermina(DBUtenti * database);

#endif
