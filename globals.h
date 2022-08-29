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
 * Il file header globals contiente strutture dati e variabili globali
 * a tutto il server.
 * 
 * @file globals.h
 * @brief Modulo connessioni del server
 *
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

//Includes
#include <sys/select.h>//per effettuare la select

#include "parser.h" //per la dichiarazione del dizionario da cui leggere le info
#include "threadpool.h"//per la dichiarazione della threadpool dei thread worker
#include "listaonline.h"//per la dichiarazione della lista di utenti online al momento
#include "dbutenti.h"//per la dichiarazione del database di utenti e messaggi
#include "connectionSocketLocks.h"//per la dichiarazione dei lock di scrittura sulle socket

extern DizionarioCompilato * dizionarioConfigChatty;
extern Threadpool * threadpoolWorkersChatty;
extern ListaOnline * listaUtentiOnlineChatty;
extern DBUtenti * databaseUtentiChatty;

//Variabili globali necessarie al funzionamento del server e dell'handler

extern long FDSocketServer;	//rappresenta il file descriptor da cui il server accetta connessioni
extern struct sockaddr_un indirizzoServer;//struct che rappresenta l'indirizzo AF_UNIX del server

extern fd_set setPrincipale;//set di file descriptors necessari per la select delle connessioni
extern fd_set setSelect;

long fdMassimo;//massimo file descriptor presente sul set

extern pthread_mutex_t setLock; //lock per gestire la concorrenza sul set di fd

//Variabili globali che vengono recuperate dal parsing del config

extern char * pathSocket;
extern char * pathFiles;
extern char * pathStatistiche;

extern int maxConnessioni;
extern int threadsInPool;
extern int dimensioneMaxMessaggio;
extern int dimensioneKByteMaxFile;
extern int dimensioneMaxHistory;

//Lock sulle socket per evitare sovrascritture

extern pthread_mutex_t * connectionSocketLocksChatty;

extern int flagTerminazioneServer; //flag per segnalare la terminazione del server

extern pthread_t threadGestioneSegnali;

#endif
