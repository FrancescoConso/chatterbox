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

#define _POSIX_C_SOURCE 200809L

#include <dbutenti.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
/*
 * il sorgente C di icl_hash è stato parzialmente modificato
 * per correggere un bug nella rimozione di un nodo dalla tabella
 * e per rendere visibile hash_pjw per il calcolo del bucketLock
 */

#include <icl_hash.h>
#include <stats.h>



/*
 * Variabili globali
 * dimensioneHistory	 - dimensione della history di un utente
 * numeroLock 			 - numero delle lock sulla tabella hash
 */
static int dimensioneHistory;
static int numeroLock;


/** @function dbUtentiHashLiberaNickname
 * 
 * @brief libera l'area di memoria che contiene il nickname
 * 
 * @param nickname	l'indirizzo del nickname da liberare
 * 
 */

void dbUtentiHashLiberaNickname(void * nickname)
{	free(nickname);	}

/** @function dbUtentiHashLiberaHistory
 * 
 * @brief libera l'area di memoria che contiene la history dell'utente
 * 
 * @param history	l'indirizzo della regione che contiene la history
 * 
 */

void dbUtentiHashLiberaHistory(void * history)
{	
	ListaHistory * historyDaLiberare = (ListaHistory *) history;	
	if(historyDaLiberare != NULL)
	{
		for(int i = (historyDaLiberare -> dimensione)-1; i > -1; i--)
		{
			free(historyDaLiberare -> messaggi[i] . data . buf);	//segmentation fault qui
		}
		free(historyDaLiberare -> messaggi);
		free(historyDaLiberare);
	}
	
}

/** @function dbUtentiInit
 * 
 * @brief inizializza il database Utenti
 * 
 * @param	numeroBuckets			numero di bucketLock nella tabella hash
 * @param	dimensioneHistoryInit	dimensione massima della history di un utente
 * @param	numeroLockInit			numero di lock sulla tabella hash
 * 
 * @return	DBUtenti se ha avuto successo
 * 			NULL altrimenti
 * 
 */

DBUtenti * dbUtentiInit(int numeroBuckets, int dimensioneHistoryInit, int numeroLockInit)
{	
	DBUtenti * dbDaCreare = malloc(sizeof(DBUtenti));
	if(dbDaCreare == NULL) {fprintf(stderr, "Errore nell'allocazione del database utenti\n"); return NULL;}
	
	//costruisco la tabella hash
	dbDaCreare -> hash = icl_hash_create(numeroBuckets, NULL, NULL);	//con NULL si utilizzano le funzioni di default di icl_hash, che sono sufficienti allo scopo
	if(dbDaCreare -> hash == NULL) {fprintf(stderr, "Errore nell'allocazione della tabella hash del database utenti\n"); return NULL;}
	dimensioneHistory = dimensioneHistoryInit;
	
	//costruisco le lock
	numeroLock = numeroLockInit;
	dbDaCreare -> lock = malloc(sizeof(pthread_mutex_t)*numeroLock);
	if(dbDaCreare -> lock == NULL) {fprintf(stderr, "Errore nell'allocazione delle lock del database utenti\n"); return NULL;}
	//attivo le lock
	for(int i=0; i<numeroLock; i++)
	if(pthread_mutex_init(&dbDaCreare -> lock[i], NULL) != 0) {fprintf(stderr, "Errore nell'inizializzazione delle lock del database utenti\n"); return NULL;}
	
	
	return dbDaCreare;	}	

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

int dbUtentiRegistraNickname(DBUtenti * database, char * nickname)		
{
	//mi impossesso della lock
	unsigned int bucketLock = hash_pjw((void *)nickname) % numeroLock;
	if(pthread_mutex_lock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}		
	
	//devo controllare che non esista già
	if(icl_hash_find(database -> hash, nickname) != NULL)//già registrato
	{
		if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
		return 0;}
	
	//allocare la lista history
	ListaHistory * historyNuovoNodo = malloc(sizeof(ListaHistory));
	if(historyNuovoNodo == NULL) {fprintf(stderr, "Errore nella allocazione della history del nuovo utente\n"); return 0;}
	
	char * nuovoNickname = strndup(nickname, 33);
	
	historyNuovoNodo -> testa = -1;
	historyNuovoNodo -> coda = -1;
	historyNuovoNodo -> dimensione = 0;
	
	historyNuovoNodo -> messaggi=malloc(sizeof(message_t) * dimensioneHistory);
	if(historyNuovoNodo -> messaggi == NULL) {fprintf(stderr, "Errore nella allocazione dei messaggi della history del nuovo utente\n"); return 0;}
	
	//ho tutto pronto, posso pushare nella hash
	
	icl_entry_t * testentry = icl_hash_insert(database -> hash, nuovoNickname, historyNuovoNodo);
	if(testentry == NULL) {fprintf(stderr, "Errore nell'inserimento del nuovo utente\n"); return 0;}
	
	//incremento la variabile al fine della statistica
	chattyStats . nusers ++;
	
	if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
	
	if(testentry != NULL)	return 1;
	else	 				return 0;
}

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

int dbUtentiDeregistraNickname(DBUtenti * database, char * nickname)
{
	unsigned int bucketLock = hash_pjw((void *)nickname) % numeroLock;
	if(pthread_mutex_lock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}
	//decremento la variabile al fine della statistica
	if (chattyStats . nusers != 0) 
		chattyStats . nusers --;
	if(icl_hash_delete(database -> hash, nickname, dbUtentiHashLiberaNickname , dbUtentiHashLiberaHistory) == 0) 
	{if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
		return 1;}
	
	else
	{if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
		fprintf(stderr, "Errore nella deregistrazione dell' utente\n");
		return 0;}
}

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

int dbUtentiCercaNickname(DBUtenti * database, char * nickname)
{
	unsigned int bucketLock = hash_pjw((void *)nickname) % numeroLock;
	if(pthread_mutex_lock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}
	
	if(icl_hash_find(database -> hash, nickname) != NULL)
	{if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
		return 1;}
	
	else
	{if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
		return 0;}
}



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

int dbUtentiAggiungiHistory (ListaHistory * historyUtente, message_t * messaggio)	//ritorna 1 se ha successo, 0 altrimenti
{
	
	//qui devo duplicare il messaggio
	message_t * nuovoMessaggio = malloc(sizeof(message_t));
	
	/*	SE QUESTI HANNO LUNGHEZZA PREFISSATA NOI LA PREFISSEREMO	*/													
	char * nuovoSender = strndup((*messaggio) . hdr . sender, 33);
	char * nuovoReceiver = strndup((*messaggio) . data . hdr . receiver, 33);
	
	
	char * nuovoBuffer = strndup((* messaggio) . data . buf, (* messaggio) . data . hdr . len);

	setHeader( &(*nuovoMessaggio) . hdr, (*messaggio) . hdr . op , nuovoSender);
	setData( &(*nuovoMessaggio) . data, nuovoReceiver, nuovoBuffer, (*messaggio) . data . hdr . len);

	
	//Così funziona, per pietà di dio non toccarla
	
	//se != NULL ho accesso alla history, posso inserire nella lista circolare
	//primo check, siamo pieni? Nel caso sovrascriviamo l'elemento di Testa e la shiftiamo
	if((historyUtente -> testa == (historyUtente -> coda + 1) % (dimensioneHistory)) || ((historyUtente -> testa == 0 && historyUtente -> coda == dimensioneHistory -1)))
	{	
		historyUtente -> testa = (historyUtente -> testa +1) % dimensioneHistory;
		historyUtente -> coda = (historyUtente -> coda +1) % dimensioneHistory;
		
		free(historyUtente -> messaggi [historyUtente -> coda] . data . buf);
		
		historyUtente -> messaggi [historyUtente -> coda] = *nuovoMessaggio;
	}
	else
	{
		if(historyUtente -> testa == -1) {historyUtente -> testa = 0;	}
																		
		historyUtente -> coda = (historyUtente -> coda +1) % dimensioneHistory;
		
		historyUtente ->  messaggi [historyUtente -> coda] = *nuovoMessaggio;
		historyUtente -> dimensione++;
	}
	
	//devo controllare che tipo di messaggio abbiamo e incrementare di conseguenza le variabili per la stat
	if((*messaggio) . hdr . op == TXT_MESSAGE) chattyStats . nnotdelivered ++;
	if((*messaggio) . hdr . op == FILE_MESSAGE) chattyStats . nfilenotdelivered ++;
	
																		free(nuovoSender);		
																		free(nuovoReceiver);	
																		
																		free(nuovoMessaggio);	
	return 1;
}

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

ListaHistory * dbUtentiRitrovaHistory(DBUtenti * database, char * nickname)	//ritorna la lista history se ha successo NULL altrimenti
{	
	//Ritrova History viene lanciata all'interno di InviaMessaggio, la lock è già stata acquisita
	//unsigned int bucketLock = hash_pjw((void *)nickname) % numeroLock;
	//pthread_mutex_lock(&database -> lock[bucketLock]);
	
	ListaHistory * daRestituire = icl_hash_find(database -> hash, nickname);
	
	//pthread_mutex_unlock(&database -> lock[bucketLock]);
	return daRestituire;
}


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
/*
ListaHistory * dbUtentiCopiaHistory(DBUtenti * database, char * nickname)	//facciamo una copia per evitare di occupare la lock durante l'invio
{
	unsigned int bucketLock = hash_pjw((void *)nickname) % numeroLock;
	pthread_mutex_lock(&database -> lock[bucketLock]);
	
	//facciamo una copia 1 a 1, quindi anche i messaggi
	ListaHistory * copiaHistory = malloc(sizeof(ListaHistory));
	ListaHistory * historyUtente = icl_hash_find(database -> hash , nickname);
	
	//cambio allocando solo la history del nickname
	//copiaHistory -> messaggi = malloc(sizeof(message_t) * dimensioneHistory);	//qui mi alloca il massimo numero di messaggi
	copiaHistory -> messaggi = malloc(sizeof(message_t) * historyUtente -> dimensione);
	
	//copio dati
	
	copiaHistory -> testa = historyUtente -> testa;
	copiaHistory -> coda = historyUtente -> coda;
	copiaHistory -> dimensione = historyUtente -> dimensione;
	
	//copio messaggi
	for(int i = 0; i < copiaHistory -> dimensione; i++)
	{
		copiaHistory -> messaggi[i] . hdr . op = historyUtente -> messaggi[i] . hdr . op;
		strncpy(copiaHistory -> messaggi[i] . hdr . sender, historyUtente -> messaggi[i] . hdr . sender, 33);
		
		strncpy(copiaHistory -> messaggi[i] . data . hdr . receiver ,historyUtente -> messaggi[i] . data . hdr . receiver, 33);
		copiaHistory -> messaggi[i] . data . hdr . len = historyUtente -> messaggi[i] . data . hdr . len;
		copiaHistory -> messaggi[i] . data . buf = strndup(historyUtente -> messaggi[i] . data . buf, historyUtente -> messaggi[i] . data . hdr . len);
	}
	
	historyUtente = NULL;
	free(historyUtente);
	
	pthread_mutex_unlock(&database -> lock[bucketLock]);
	
	return copiaHistory;
	
}*/

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

int dbUtentiInviaMessaggio(DBUtenti * database, op_t op, char * sender, char * receiver, char * buf, unsigned int len )	//1 se ha successo, 0 altrimenti
{
	unsigned int bucketLock = hash_pjw((void *)receiver) % numeroLock;
	if(pthread_mutex_lock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}			
	
	message_t * messaggioDaInviare = malloc(sizeof(message_t));
	if(messaggioDaInviare == NULL) {fprintf(stderr, "Errore nell'allocazione di un messaggio del database utenti\n"); return 0;}		
	
	setHeader(&(*messaggioDaInviare).hdr, op, sender);
	setData(&(*messaggioDaInviare).data, receiver, buf, len);
	
	ListaHistory * historyUtente;
	if((historyUtente = dbUtentiRitrovaHistory(database, receiver)) == NULL) {free(messaggioDaInviare);	return 0;}
	if(dbUtentiAggiungiHistory(historyUtente, messaggioDaInviare) == 0) {fprintf(stderr, "Errore nell'aggiunta del messaggio alla history dell'utente\n"); return 0;}
	
	free(messaggioDaInviare);
	
	if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
	
	return 1;
}

/** @function dbUtentiBroadcastMessaggio
 * 
 * @brief aggiunge alla history di tutti gli utenti meno il mittente un messaggio
 * 
 * @param database		database su cui cercare la history
 * @param sender		mittente del messaggio
 * @param buf			testo del messaggio
 * @param len			lunghezza del testo
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiBroadcastMessaggio (DBUtenti * database, op_t op, char * sender, char * buf, unsigned int len) //il broadcast è l'operazione 3
{
	//iterazione sulla hash table basata sul metodo icl_hash_dump
	//l'header del messaggio è uguale per tutti
	message_t * messaggioBroadcast = malloc(sizeof(message_t));
	if(messaggioBroadcast == NULL) {fprintf(stderr, "Errore nell'allocazione di un messaggio per il broadcast del database utenti\n"); return 0;}	
	
	setHeader(&(*messaggioBroadcast).hdr, op, sender);
	
	//itero sulla hash table, valorizzando di volta in volta la parte data e pushando il messaggio corrispondente nella history
	icl_entry_t *bucketLock, *curr;	
    int i;
	
	icl_hash_t * hash = database -> hash;
	
    if(!hash) return 0;

    for(i=0; i<hash->nbuckets; i++) {
        bucketLock = hash->buckets[i];
        for(curr=bucketLock; curr!=NULL; ) {
            if(curr->key && strcmp(curr -> key, sender) != 0)
            {
				setData(&(*messaggioBroadcast).data, curr->key, buf, len);
				
				unsigned int bucketLock = hash_pjw((void *)(*messaggioBroadcast).data.hdr.receiver) % numeroLock;
				if(pthread_mutex_lock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}		
				
                dbUtentiAggiungiHistory(curr -> data, messaggioBroadcast);
                
                if(pthread_mutex_unlock(&database -> lock[bucketLock]) != 0) {fprintf(stderr, "Errore nella liberazione della lock del database utenti\n"); return 0;}	
                
			}
            curr=curr->next;
        }
    }
    free(messaggioBroadcast);
    
    
    
	return 1;
}

/** @function dbUtentiTermina
 * 
 * @brief termina e dealloca il dbutenti
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int dbUtentiTermina(DBUtenti * database)
{	
	//distruggo la hash
	if(icl_hash_destroy(database -> hash, dbUtentiHashLiberaNickname, dbUtentiHashLiberaHistory) != 0) {fprintf(stderr, "Errore nella distruzione della hash del database utenti\n"); return 0;}	
	
	//distruggo le lock
	for(int i=0; i<numeroLock; i++)	{
		
		//if(pthread_mutex_lock(&database -> lock[i]) != 0) {fprintf(stderr, "Errore nell'acquisizione della lock del database utenti\n"); return 0;}
		if(pthread_mutex_destroy(&database -> lock[i]) != 0) {fprintf(stderr, "Errore nella distruzione della lock del database utenti\n"); return 0;}			
	}
	free(database -> lock);
	
	//distruggo la struct
	if(database != NULL)free(database);
	
	return 1;
}
