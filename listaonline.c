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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <listaonline.h>
#include <stats.h>
#include <config.h>

#include <stdlib.h>

/* Variabili globali
 * numeroNodi	conta il numero di nodi della lista online
 */
static int numeroNodi;


/** @function listaOnlineInit
 * 
 * @brief inizializza la lista online
 * 
 * @return	ListaOnline se ha avuto successo
 * 			NULL altrimenti
 * 
 */

ListaOnline * listaOnlineInit()
{
	ListaOnline * lista = malloc(sizeof(ListaOnline));
	memset(lista, 0, sizeof(ListaOnline));//CRITICO	//ERROR
	
	if(lista == NULL) {fprintf(stderr, "Errore nell'allocazione della lista online\n"); return NULL;}
	numeroNodi = 0;
	if(pthread_mutex_init(&lista -> lockLista, NULL) != 0) {fprintf(stderr, "Errore nell'inizializzazione della lock della lista online\n"); return NULL;}
	
	//lista -> param=malloc(sizeof(ParametriListaOnline));
	//if(lista -> param == NULL) 	{fprintf(stderr, "Errore nell'allocazione dei parametri della lista online\n"); return NULL;}

	return lista;
}

/** @function listaOnlineNodoUtenteAggiungi
 * 
 * @brief aggiunge un utente alla lista online
 * 
 * @param lista		lista a cui aggiungere un utente
 * @param nickname	nickname dell'utente
 * @param socket	socket su cui l'utente è connesso
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */

int listaOnlineNodoUtenteAggiungi(ListaOnline * lista, char * nickname, int socket)
{
																		
	//controllo che l'utente non sia già stato inserito, se lo è stato salto il processo
	int risRicerca = -1;
	risRicerca = listaOnlineNodoUtenteOttieniSocket(lista, nickname);
	if (risRicerca == -1)
	{
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return 0;}
	
		
		NodoUtenteOnline * newNodo = malloc(sizeof(NodoUtenteOnline));
		if(!newNodo){fprintf(stderr, "Errore nell'allocazione del nuovo nodo\n"); return 0;}
		
		
		newNodo -> nickname= malloc(sizeof(char) * MAX_NAME_LENGTH + 1);	//CRITICO NON CAMBIARE DIMENSIONE NICKNAME
		
		if(!newNodo -> nickname){fprintf(stderr, "Errore nell'allocazione del nuovo nodo - nickname\n"); return 0;}
		strncpy(newNodo -> nickname, nickname, MAX_NAME_LENGTH + 1);
		
		
		newNodo -> socket = socket;
		newNodo -> next = NULL;
		
		
		if (lista -> listaOnline == NULL)	{lista -> listaOnline = newNodo;}
		else
		{		
			newNodo -> next = lista -> listaOnline;
			lista -> listaOnline = newNodo;
		}
		
		
		numeroNodi ++;	
		//incremento la variabile a fine statistico
		chattyStats . nonline ++;															
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return 0;}
	
	}
	return 1;
}

/** @function listaOnlineNodoUtenteRimuovi
 * 
 * @brief rimuove un utente alla lista online in base al socket
 * 
 * @param lista		lista da cui rimuovere l' utente
 * @param socket	socket su cui l'utente è connesso
 * 
 * @return	1 se ha avuto successo
 * 			-1 se l'utente non esiste
 * 			0 altrimenti
 * 			
 */
 
int listaOnlineNodoUtenteRimuovi(ListaOnline * lista, int socketDisconnesso)
{
	int risultatoRimozione = -1;
	char * nicknameDisconnesso = listaOnlineNodoUtenteOttieniNickname(lista, socketDisconnesso);
	if (nicknameDisconnesso != NULL)
	{
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return 0;}
		NodoUtenteOnline * iteratorePrecedente = NULL;
		NodoUtenteOnline * iteratoreCorrente = NULL;

		
		//Fase 1: itero la lista e cancello utenti connessi sul socket
		//1.1 il primo elemento è disconnesso?
		/*
		while((lista -> listaOnline != NULL) && (lista -> listaOnline -> socket == socketDisconnesso))
		{
			NodoUtenteOnline * vecchiaTesta = lista -> listaOnline;
			lista -> listaOnline = lista -> listaOnline -> next;
			free(vecchiaTesta -> nickname);	//CRITICO	//GRIND
			free(vecchiaTesta);
			risultatoRimozione = 1;
		}
		
		if ( lista -> listaOnline != NULL) //Se la lista online fosse vuota è inutle iterare
		
		}*/
		
		
		
		if (lista -> listaOnline != NULL)
		{
			//1.1 il primo elemento è disconnesso?
			if(lista -> listaOnline -> socket == socketDisconnesso)
			{
				NodoUtenteOnline * vecchiaTesta = lista -> listaOnline;
				lista -> listaOnline = lista -> listaOnline -> next;
				free(vecchiaTesta -> nickname);	//CRITICO	//GRIND
				free(vecchiaTesta);
				risultatoRimozione = 1;
			}
			else
			{
				iteratorePrecedente = lista -> listaOnline;
				iteratoreCorrente = lista -> listaOnline -> next;
				
				//1.2 scorro tutti i nodi fino all'ultimo
				while(iteratoreCorrente != NULL)
				{
					if(strncmp(iteratoreCorrente -> nickname, nicknameDisconnesso, MAX_NAME_LENGTH + 1) == 0)
					{
						iteratorePrecedente -> next = iteratoreCorrente -> next;
						risultatoRimozione = 1;
						break;
					}
					else
					{
						iteratorePrecedente = iteratoreCorrente;
						iteratoreCorrente = iteratoreCorrente -> next;
					}
				}
				free(iteratoreCorrente -> nickname);	//CRITICO	//GRIND
				free(iteratoreCorrente);				//CRITICO	//GRIND
			}
		
		numeroNodi --;
		if (chattyStats . nonline != 0) chattyStats . nonline --;
		}
		
		
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return 0;}
	}
	return risultatoRimozione;
}
																	
/** @function listaOnlineRitrovaParametri
 * 
 * @brief restituisce i parametri della lista online
 * 
 * @param lista	lista utenti online
 * 
 * @return	ParametriListaOnline se ha avuto successo
 * 			NULL altrimenti
 * 
 */

ParametriListaOnline * listaOnlineRitrovaParametri(ListaOnline * lista)
{
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return NULL;}
	
	//int contatoreOnline=0;
	ParametriListaOnline * daRestituire = malloc(sizeof(ParametriListaOnline));
	if(!daRestituire){fprintf(stderr, "Errore nell'allocazione dei parametri della lista online\n"); return NULL;}
	daRestituire -> len = 0;
	daRestituire -> buf = NULL;
	
	NodoUtenteOnline * iteratoreCorrente = NULL;

	//ora calcolo lunghezza, alloco buffer e valorizzo tutto
	
	int len = (numeroNodi * (MAX_NAME_LENGTH+1));
	daRestituire -> buf = malloc(sizeof(char) * len);
	if(!daRestituire -> buf){fprintf(stderr, "Errore nell'allocazione del buffer\n"); return NULL;}
	daRestituire -> len = len;
	
	//itero la lista per costruire il buffer
	
	iteratoreCorrente = lista -> listaOnline;
	//if(iteratoreCorrente == NULL) {fprintf(stderr, "Errore nell'iterazione della lista online\n"); return NULL;}
	
	char * nicknameBufferizzato = malloc(sizeof(char) * (MAX_NAME_LENGTH+1));
	if(!nicknameBufferizzato){fprintf(stderr, "Errore nella bufferizzazione della lista\n"); return NULL;}
	int iteratoreBuffer=0;
	
	while(iteratoreCorrente != NULL)
	{	
		strncpy(nicknameBufferizzato, iteratoreCorrente -> nickname, (MAX_NAME_LENGTH+1));
		nicknameBufferizzato[MAX_NAME_LENGTH]='\0';
		//nicknameBufferizzato ora contine la lunghezza che serve
		strncpy(&daRestituire -> buf[iteratoreBuffer], nicknameBufferizzato, (MAX_NAME_LENGTH+1));
		iteratoreBuffer=iteratoreBuffer+(MAX_NAME_LENGTH+1);
		
		iteratoreCorrente = iteratoreCorrente -> next;
	}
	
	//valori di default in caso di fallimento?
	
	if(nicknameBufferizzato != NULL) free(nicknameBufferizzato);
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return NULL;}
	return daRestituire;						

}

/** @function listaOnlineRitrovaNumeroOnline
 * 
 * @brief restituisce il numero di utenti online nella lista
 * 
 * @param lista	lista utenti online
 * 
 * @return	numeroNodi se ha avuto successo
 * 			-1 altrimenti
 * 
 */
int listaOnlineRitrovaNumeroOnline (ListaOnline * lista)
{
	//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return -1;}
	
	int daRestituire = numeroNodi;
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return -1;}

	return daRestituire;

}

/** @function listaOnlineRitrovaBufferOnline
 * 
 * @brief restituisce il buffer contenente i nomi degli utenti online
 * 
 * @param lista	lista utenti online
 * 
 * @return	buf se ha avuto successo
 * 			NULL altrimenti
 * 
 */
char * listaOnlineRitrovaBufferOnline (ListaOnline * lista)
{
	ParametriListaOnline * parametriOnline = malloc(sizeof(ParametriListaOnline *));
	parametriOnline = listaOnlineRitrovaParametri(lista);
	
	char * bufferUtentiOnline = parametriOnline -> buf;
	
	//free(parametriOnline -> buf);
	free(parametriOnline);
	
	return bufferUtentiOnline;
	
	return NULL;
}



/** @function listaOnlineNodoUtenteOttieniSocket
 * 
 * @brief restituisce il socket di un utente con un certo nickname
 * 
 * @param lista		lista utenti online
 * @param nickname 	nickname dell'utente
 * 
 * @return	numero socket se ha avuto successo
 * 			-1 se la socket non viene trovata
 * 			0 altrimenti
 * 
 */

int listaOnlineNodoUtenteOttieniSocket(ListaOnline * lista, char * nicknameDaCercare)
{
	int socketTrovato = -1;
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return 0;}

		NodoUtenteOnline * iteratoreCorrente = lista -> listaOnline;
		//if(iteratoreCorrente == NULL) {fprintf(stderr, "Errore nell'iterazione della lista online\n"); return 0;}
		
		while(iteratoreCorrente != NULL)
		{	
			if(strcmp(iteratoreCorrente -> nickname, nicknameDaCercare) == 0)
			{
				socketTrovato = iteratoreCorrente -> socket;
				break;
			}
			else iteratoreCorrente = iteratoreCorrente -> next;
		}

																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return 0;}
	return socketTrovato;
}

/** @function listaOnlineNodoUtenteOttieniNickname
 * 
 * @brief restituisce il nickname di un utente su una certa socket
 * 
 * @param lista		lista utenti online
 * @param nickname 	nickname dell'utente
 * 
 * @return	nickname se ha avuto successo
 * 			NULL altrimenti
 * 
 */

char * listaOnlineNodoUtenteOttieniNickname(ListaOnline * lista, int socketDaCercare)
{
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return NULL;}
	NodoUtenteOnline * iteratoreCorrente = NULL;
	char * nicknameTrovato = NULL;
	
	//itero la lista concatenata, per contare gli utenti online
	
	iteratoreCorrente = lista -> listaOnline;
	//if(iteratoreCorrente == NULL) {fprintf(stderr, "Errore nell'iterazione della lista online\n"); return 0;}
	
	while(iteratoreCorrente != NULL)
	{	
		if(iteratoreCorrente -> socket == socketDaCercare)
		{
			nicknameTrovato = iteratoreCorrente -> nickname;
			break;
		}
		else iteratoreCorrente = iteratoreCorrente -> next;
	}
	
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return NULL;}
	return nicknameTrovato;
}



/** @function listaOnlineTermina
 * 
 * @brief libera la memoria della listaOnline
 * 
 * @param lista		lista utenti online
 * 
 * @return	1 se l'operazione ha avuto successo
 * 			0 altrimenti
 * 
 */

int listaOnlineTermina(ListaOnline * lista)
{
	if(lista == NULL) {fprintf(stderr, "Errore di accesso alla lista per la terminazione\n"); return 0;}
																		//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return 0;}
	//ora posso deallocare la lista
	//prima dealloco tutti i nodi della lista online
	NodoUtenteOnline * iteratoreCorrente = lista -> listaOnline;
	//if(iteratoreCorrente == NULL) {fprintf(stderr, "Errore di accesso all'iteratore per la terminazione\n"); return 0;}
	while(iteratoreCorrente != NULL)
	{
		lista -> listaOnline = lista -> listaOnline -> next;
		free(iteratoreCorrente -> nickname);
		free(iteratoreCorrente);
		iteratoreCorrente = lista -> listaOnline;
	}
	//poi dealloco i parametri
	//if(lista -> param == NULL) {fprintf(stderr, "Errore di accesso ai parametri per la terminazione\n"); return 0;}
	//if(lista -> param -> buf == NULL) {fprintf(stderr, "Errore di accesso al parametro buffer per la terminazione\n"); return 0;}
	//if(lista -> param != NULL) free(lista -> param -> buf);
	//if(lista -> param != NULL) free(lista -> param);
	if(lista -> param != NULL) free(lista -> param -> buf);
	free(lista -> param);
	//poi la lock	
	pthread_mutex_destroy(&(lista -> lockLista));
	
	free(lista);

	
	
	return 1;
	
}
