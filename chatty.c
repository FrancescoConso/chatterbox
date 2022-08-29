/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>

/* inserire gli altri include che servono */
#include <stats.h>
#include <globals.h>
#include <connections.h>
#include <threadpool.h>
#include <parser.h>

#include <message.h>
/* struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 *
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };


/* strutture dati globali del server, definite in globals.h */
DizionarioCompilato * dizionarioConfigChatty;
Threadpool * threadpoolWorkersChatty;
ListaOnline * listaUtentiOnlineChatty;
DBUtenti * databaseUtentiChatty;
pthread_mutex_t * connectionSocketLocksChatty;

char * pathSocket;
char * pathFiles;
char * pathStatistiche;	

int maxConnessioni;
int threadsInPool;
int dimensioneMaxMessaggio;
int dimensioneKByteMaxFile;
int dimensioneMaxHistory;

long FDSocketServer;

struct sockaddr_un indirizzoServer;

fd_set setPrincipale;
fd_set setSelect;

pthread_mutex_t setLock;

int flagTerminazioneServer;

pthread_t threadGestioneSegnali;

/*
 * 
 * 
 * METODI DI APPOGGIO
 * 
 * 
 * */

int charToInt (char * numero)
{
	int cifre = strlen(numero);
	int signif = 1;
	int accumul = 0;
	
	//partendo dalla cifra meno significativa scrivo sull'accumulatore il
	//valore complessivo
	
	for(int i = cifre-1; i>=0; i--)
	{
		accumul = accumul + ((numero[i] - 48) * signif);
		signif = signif * 10;
	}
	
	return accumul;
}

int threadpoolInoltraFD(long FDClient)
{
	int risIncoda = threadpoolCodaPendentiIncodaTask(threadpoolWorkersChatty, FDClient); return risIncoda;
}


int serverChattyInit(char * configFile)
{
	//Inizializzo tutti i moduli dati necessari, se ci fossero problemi restituisco -1
	
	dizionarioConfigChatty = parserDizionarioInit(configFile);
	if(dizionarioConfigChatty == NULL) return -1;
	//dal parse del dizionario si ricavano i valori
	
	pathSocket = (char *) parserDizionarioLookupDefinizione("UnixPath", dizionarioConfigChatty);
	pathFiles = (char *) parserDizionarioLookupDefinizione("DirName", dizionarioConfigChatty);
	pathStatistiche = (char *) parserDizionarioLookupDefinizione("StatFileName", dizionarioConfigChatty);
	
	//questi int sono in realtà char e vanno convertiti, poco elegante ma va fatto
	
	maxConnessioni = charToInt( parserDizionarioLookupDefinizione("MaxConnections", dizionarioConfigChatty) );
	threadsInPool = charToInt( parserDizionarioLookupDefinizione("ThreadsInPool", dizionarioConfigChatty) );
	dimensioneMaxMessaggio = charToInt( parserDizionarioLookupDefinizione("MaxMsgSize", dizionarioConfigChatty) );
	dimensioneKByteMaxFile = charToInt( parserDizionarioLookupDefinizione("MaxFileSize", dizionarioConfigChatty) );
	dimensioneMaxHistory = charToInt( parserDizionarioLookupDefinizione("MaxHistMsgs", dizionarioConfigChatty) );

	listaUtentiOnlineChatty = listaOnlineInit();
	if(listaUtentiOnlineChatty == NULL) return -1;
	
	//dimensione database utenti:
	//numero di bucket - maxConnessioni
	//dimensione della history di ciascun utente - data dal config
	//dimensione dei lock della tabella - threadsInPool
	databaseUtentiChatty = dbUtentiInit(maxConnessioni, dimensioneMaxHistory, threadsInPool);
	if(databaseUtentiChatty == NULL) return -1;
	
	threadpoolWorkersChatty = threadpoolInit(threadsInPool);
	if(threadpoolWorkersChatty == NULL) return -1;
	
	connectionSocketLocksChatty = connectionSocketLocksInit(threadsInPool);
	if(connectionSocketLocksChatty == NULL) return -1;
	int risMutex = pthread_mutex_init(&setLock, NULL);
	if( risMutex != 0)return -1;
	
	flagTerminazioneServer = 0;
	
	return 1;	//1 in caso di successo
}



int serverChattyAvvia()	//associo socket e indirizzo e segnalo di essere pronto a ricevere
{
	int ris;	//variabile che controlla i risultati delle operazioni sulle socket
	
	//creazione socket del server
	FDSocketServer = socket(AF_UNIX, SOCK_STREAM, 0);
	
	if (FDSocketServer == -1)	{	perror("socket");	exit(errno);	}
	
	//valorizzazione famiglia e nome socket
	indirizzoServer.sun_family = AF_UNIX;
    strncpy(indirizzoServer.sun_path, pathSocket, strlen(pathSocket)+1);
    unlink(pathSocket);
    
    //associazione socket ad indirizzo
    ris = bind(FDSocketServer, (struct sockaddr*) &indirizzoServer, sizeof(indirizzoServer));
	if (ris == -1)	{	perror("bind");	exit(errno);	}
    
    //segnalo di essere pronto a ricevere
    ris = listen(FDSocketServer, BUFFER_SOCKET);
	if (ris == -1)	{	perror("listen");	exit(errno);}	
	
	return 1;
}

void serverChattyWhile() //qui dentro c'è il while 1 del server
{
	//azzero i set
	FD_ZERO(&setPrincipale);
    FD_ZERO(&setSelect);	
	//aggiungo il fd del listener al set principale
	FD_SET(FDSocketServer, &setPrincipale);	

	//è necessario tenere traccia dell'fd più grande - inizialmente valorizzato al listener
	fdMassimo = FDSocketServer;	
	struct timeval timeout;
	while(flagTerminazioneServer == 0)	//loop gestione connessioni
	{
		int timeoutSelect;	//risultati dell'attesa della select
		long FDSocketDati;
		timeout.tv_sec = 0;
		timeout.tv_usec = USEC_TIMEOUT;
		

		//si copia il set principale nel set temporaneo per la select
		pthread_mutex_lock(&setLock);	//blocco il set per evitare che altre operazioni lo modifichino
		setSelect = setPrincipale;
		//faccio la select che comincia a scansionare il set in attesa che arrivino
		//richieste da un thread (il codice sta fermo qui fino all'arrivo di richieste)
		timeoutSelect = select(fdMassimo+1, &setSelect, NULL, NULL, &timeout);	//la select seleziona un fd dal quale aspetta di ricevere comandi
		pthread_mutex_unlock(&setLock);																
		
		if (timeoutSelect == -1) { 
			if (errno == EINTR)	//se il server è stato chiuso termino con successo
			{exit(EXIT_SUCCESS);}
			else
			perror("select");	exit(errno); }//errore della select
		
		if(timeoutSelect)	//è stato scelto con successo un fd
		{		
			if(FD_ISSET(FDSocketServer, &setSelect))
			{	
				FDSocketDati = accept(FDSocketServer, NULL, NULL);
				//printf("C'E' UN CLIENT SU: %li\n", FDSocketDati);
				if (FDSocketDati == -1 ) { perror("accept");	exit(errno);}
					
				//printf("Accettato client FD %d \n", FDSocketDati);
					
				//aggiungo l'fd al set e se necessario reimposto il fd massimo
				pthread_mutex_lock(&setLock);
				
				//inoltro il file descriptor alla threadpool
				if(threadpoolInoltraFD(FDSocketDati) == 0) {	perror("threadpool");	exit(EXIT_FAILURE);	};
				
				FD_SET(FDSocketDati, &setPrincipale);
				if(FDSocketDati > fdMassimo)	fdMassimo = FDSocketDati; 
					
				pthread_mutex_unlock(&setLock);	
				continue;
			}	
		}//timeoutselect != 0
		
		else //sono in timeout
		{ continue; }
		
		
		
	}//chiusura while
	
	//printf("IL MIO TEMPO COME SELECT E' TERMINATO\n");
}

void serverChattyCleanup()
{
	//chiusura connessione
	//chiudo la socket della connessione del server
	close(FDSocketServer);
	//scollego la socket
	if(pathSocket != NULL)
	unlink(pathSocket);
}

void serverChattyTermina()	//pulizia del server completa a terminazione
{
	flagTerminazioneServer = 1;
	
	serverChattyCleanup();
	
	//deallocazione strutture
	
	if (threadpoolWorkersChatty != NULL)
	{threadpoolTermina(threadpoolWorkersChatty);
		/*printf("threadpoolWorkersChatty != NULL\n");*/}	
		
	if (listaUtentiOnlineChatty != NULL)
	{listaOnlineTermina(listaUtentiOnlineChatty);
		/*printf("listaUtentiOnlineChatty != NULL\n");*/}
	
	if (databaseUtentiChatty != NULL)
	{dbUtentiTermina(databaseUtentiChatty);
		/*printf("databaseUtentiChatty != NULL\n");*/}
	
	if(dizionarioConfigChatty != NULL)
	{parserDizionarioTermina(dizionarioConfigChatty);
		/*printf("dizionarioConfigChatty != NULL\n");*/}
	
	if(	connectionSocketLocksChatty != NULL)
	{connectionSocketLocksTermina (connectionSocketLocksChatty);}
	
	
}

void serverChattyStatistiche()
{
	//apro il file in modalità append - creo se non esiste
	FILE * fileStatistiche = fopen(pathStatistiche, "a");
	if (fileStatistiche == NULL)
	{	fprintf(stderr, "Errore creazione-apertura file statistiche\n");
		exit(EXIT_FAILURE);	}
	int risStat = printStats(fileStatistiche);
	if( risStat == -1)
	{
		fprintf(stderr, "Errore append file statistiche\n");
		exit(EXIT_FAILURE);	}
	
	fclose(fileStatistiche);
}

static void * gestioneSegnaliChatty(void * setSegnali)
{
	sigset_t * segnali = setSegnali;
	int segnale;
	while(flagTerminazioneServer == 0)
	{
		sigwait(segnali, &segnale);
		switch(segnale)
		{
			case SIGUSR1:
			{	serverChattyStatistiche(); } break;
			case SIGINT:
			case SIGTERM:
			case SIGQUIT:
			{	printf ("Terminazione server per signal\n"); serverChattyTermina();	} break;
			default:
			break;
		}
	}
	return NULL;
}

/*
 * 
 * 
 * 
 * METODI FORNITI
 * 
 * 
 * 
 * */
static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

int main(int argc, char *argv[]) {
	
	serverChattyCleanup();


    sigset_t setSegnali;
    int ris;
    
	sigemptyset(&setSegnali);
	sigaddset(&setSegnali, SIGINT);
	sigaddset(&setSegnali, SIGTERM);
    sigaddset(&setSegnali, SIGQUIT);
    sigaddset(&setSegnali, SIGUSR1);
    
    ris = pthread_sigmask(SIG_BLOCK, &setSegnali, NULL);
    if (ris != 0)
    {
		perror("pthread_sigmask"); exit(EXIT_FAILURE);
	}
	
    ris = pthread_create(&threadGestioneSegnali, NULL, &gestioneSegnaliChatty, (void *) &setSegnali);
    if (ris != 0)
    {
		perror("pthread_create"); exit(EXIT_FAILURE);
	}
	
	ris = pthread_detach(threadGestioneSegnali);
    if (ris != 0)
    {
		perror("pthread_detach"); exit(EXIT_FAILURE);
	}
	
	//ignoro la sigpipe per non terminare in caso di scrittura su socket chiusa
	signal(SIGPIPE, SIG_IGN);
	
	//mi assicuro che il programma sia stato lanciato in modo appropriato
	if(argc < 3)
	{	usage(argv[0]); exit(EXIT_FAILURE);	}
	
	//inizializzazione di tutti i moduli
	if(serverChattyInit(argv[2]) == -1) {printf("Errore nell'inizializzazione del server\n"); exit(EXIT_FAILURE);}
	
	if(serverChattyAvvia() != -1)
	serverChattyWhile();	//qui il ciclo principale
	else
	{	
		printf ("Terminazione server per errore\n");
		serverChattyTermina();
		printf("Errore nell'avvio del server\n"); exit(EXIT_FAILURE); 
	}
	
	//non raggiunto
	//serverChattyTermina();
	
    return 0;
}
