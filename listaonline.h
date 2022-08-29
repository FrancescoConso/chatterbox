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

#ifndef LISTAONLINE_H_
#define LISTAONLINE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


//#include "config.h"




#include <stdlib.h>


/* Strutture dati:
 * NodoUtenteOnline		 - Nodo della lista online
 * ListaOnline			 - Struttura dati contenente gli utenti online
 * ParametriListaOnline	 - Parametri della lista online che devono essere passati al client
 */

typedef struct NodoUtenteOnline
{
	char * nickname;					//nickname utente
	int socket;							//socket su cui l'utente è connesso
	struct NodoUtenteOnline * next;		//nodo successivo della lista
} NodoUtenteOnline;

typedef struct ParametriListaOnline
{
	int len;							//lunghezza lista
	char * buf;							//contiene tutti i nickname concatenati
}	ParametriListaOnline;

typedef struct ListaOnline
{
	pthread_mutex_t lockLista;			//lock per l'accesso alla lista
	NodoUtenteOnline * listaOnline;		//lista degli utenti online
	ParametriListaOnline * param;		//parametri della lista online
} ListaOnline;


/** @function listaOnlineInit
 * 
 * @brief inizializza la lista online
 * 
 * @return	ListaOnline se ha avuto successo
 * 			NULL altrimenti
 * 
 */

ListaOnline * listaOnlineInit();


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

int listaOnlineNodoUtenteAggiungi(ListaOnline * lista, char * nickname, int socket);


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
 
int listaOnlineNodoUtenteRimuovi(ListaOnline * lista, int socketDisconnesso);

																	
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
ParametriListaOnline * listaOnlineRitrovaParametri(ListaOnline * lista);


/** @function listaOnlineRitrovaNumeroOnline
 * 
 * @brief restituisce il numero degli utenti online
 * 
 * @param lista	lista utenti online
 * 
 * @return	len se ha avuto successo
 * 			NULL altrimenti
 * 
 */
int listaOnlineRitrovaNumeroOnline (ListaOnline * lista);

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
char * listaOnlineRitrovaBufferOnline (ListaOnline * lista);


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


int listaOnlineNodoUtenteOttieniSocket(ListaOnline * lista, char * nicknameDaCercare);

/** @function listaOnlineNodoUtenteOttieniSocket
 * 
 * @brief restituisce il socket di un utente su una certa socket
 * 
 * @param lista		lista utenti online
 * @param nickname 	nickname dell'utente
 * 
 * @return	nickname se ha avuto successo
 * 			NULL altrimenti
 * 
 */

char * listaOnlineNodoUtenteOttieniNickname(ListaOnline * lista, int socketDaCercare);


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

int listaOnlineTermina(ListaOnline * lista);

#endif
