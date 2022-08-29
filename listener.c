#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

#include <message.h>
#include <connections.h>
#include <stats.h>
#include <handlerClient.h>
#include <globals.h>


int inviaNotificaMessaggio(int tipoMessaggio, int FDSocketDestinatario, message_hdr_t * header, message_data_t * dati)
{
	/*
	message_hdr_t * notifica = malloc(sizeof(message_hdr_t));
	message_data_t * messaggio = malloc(sizeof(message_hdr_t));
	
	setHeader(&(*notifica), tipoMessaggio, header -> sender);
	setData(&(*messaggio), dati -> hdr . receiver, dati -> buf ,  dati -> hdr . len );

	connectionSocketLocksLock(FDSocketDestinatario, connectionSocketLocksChatty);			
	//invio notifica
	sendHeader(FDSocketDestinatario, notifica);
	sendData(FDSocketDestinatario, messaggio);
	
	connectionSocketLocksUnlock(FDSocketDestinatario, connectionSocketLocksChatty);
	
	free(notifica);
	free(messaggio);*/
	
	message_hdr_t notifica;
	message_data_t messaggio;
	
	setHeader(&notifica, tipoMessaggio, header -> sender);
	setData(&messaggio, dati -> hdr . receiver, dati -> buf ,  dati -> hdr . len );

	connectionSocketLocksLock(FDSocketDestinatario, connectionSocketLocksChatty);			
	//invio notifica
	sendHeader(FDSocketDestinatario, &notifica);
	sendData(FDSocketDestinatario, &messaggio);
	
	connectionSocketLocksUnlock(FDSocketDestinatario, connectionSocketLocksChatty);
	
	return 1;
}


int broadcastOnlineNotifica(int tipoNotifica , ListaOnline * lista, long FDSocketMittente, message_t * messaggio)
{
	//devo inviare una notifica a tutti gli utenti online
	//acquisisco il blocco sulla lista online, la itero e invio
	//una notifica su ogni socket
	//Acquisisco lock sulla mutex
																		if(pthread_mutex_lock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nell'acquisizone della lock di mutua esclusione\n"); return 0;}
	NodoUtenteOnline * iteratoreCorrente = NULL;
	
	//itero la lista concatenata, per contare gli utenti online
	
	iteratoreCorrente = lista -> listaOnline;
	//if(iteratoreCorrente == NULL) {fprintf(stderr, "Errore nell'iterazione della lista online\n"); return 0;}
	
	while(iteratoreCorrente != NULL)
	{	
		if(iteratoreCorrente -> socket != FDSocketMittente)
		{
			//printf("(broadcast) EHI! PARE CHE: %s SIA CONNESS* SULLA SOCKET: %d \n", iteratoreCorrente->nickname ,iteratoreCorrente -> socket);
			//printf("EHI! PARE CHE: %s SIA CONNESS* SULLA SOCKET: %li\n", receiver, FDSocketDestinatario);
			//pthread_mutex_lock(&setLock);	//blocco il set per evitare l'invio di messaggi a client che si sono disconnessi
		inviaNotificaMessaggio(TXT_MESSAGE, iteratoreCorrente -> socket, & messaggio -> hdr, & messaggio -> data);
		//pthread_mutex_unlock(&setLock);	
		}
		iteratoreCorrente = iteratoreCorrente -> next;
	}
	
																		//ho finito, posso sbloccare la mutex
																		if(pthread_mutex_unlock(&(lista -> lockLista)) != 0){fprintf(stderr, "Errore nel rilascio della lock di mutua esclusione\n"); return 0;}
	return 1;
}

int inviaMessaggi(long FDSocketDestinatario, char * sender)
{
	//ottengo la history completa dei messaggi	
	ListaHistory * historyMittente = dbUtentiRitrovaHistory(databaseUtentiChatty, sender);
	
	//message_t * testaMessaggi = malloc(sizeof(message_t));
	message_t testaMessaggi;
	
	//imposto l'header del messaggio di testa
	//deve essere di tipo ok, con data.buf che contiene il numero di messaggi
	/*int * dimensioneCircBuffer = malloc(sizeof(int));
	* dimensioneCircBuffer = dimensioneCirc;
	char * numeroMessaggi = (char *) dimensioneCircBuffer;*/
	
	int * dimensioneCircBuffer = & historyMittente -> dimensione;
	char * numeroMessaggi = (char *) dimensioneCircBuffer;
	
	setHeader(&testaMessaggi.hdr, OP_OK, "");
	setData(&testaMessaggi.data, "", numeroMessaggi, strlen(numeroMessaggi)+1);
	
	
	connectionSocketLocksLock(FDSocketDestinatario, connectionSocketLocksChatty);
	sendRequest(FDSocketDestinatario, &testaMessaggi);
	
	//per ogni messaggio della history lo formatto correttamente e lo invio al client
	//message_t * iEsimoMessaggio = malloc(sizeof(message_t));
	message_t iEsimoMessaggio;	//CRITICO	//GRIND
	//la history è un array circolare
	int testaCirc = historyMittente -> testa;
	for (int i = 0; i < *dimensioneCircBuffer; i++)
	{
		
		iEsimoMessaggio = historyMittente -> messaggi[(i+testaCirc) % dimensioneMaxHistory];
		sendRequest(FDSocketDestinatario, &iEsimoMessaggio);

		if(iEsimoMessaggio . hdr . op == TXT_MESSAGE)
		{if (chattyStats . nnotdelivered != 0) 
			chattyStats . nnotdelivered -- ; 
			chattyStats . ndelivered ++ ;}
		if(iEsimoMessaggio . hdr . op == FILE_MESSAGE)
		{if (chattyStats . nfilenotdelivered != 0) 
			chattyStats . nfilenotdelivered --; 
			chattyStats . nfiledelivered ++ ;}
			
		//free(iEsimoMessaggio);	//CRITICO	//GRIND	//INVALID
		
	}
	//tutto ok
	//free(&historyMittente -> messaggi);	//CRITICO	//GRIND	//INVALID

	//free(historyMittente);

	
	connectionSocketLocksUnlock(FDSocketDestinatario, connectionSocketLocksChatty);
	return 1;
}

int inviaNotifica(int tipoNotifica, int FDSocketDestinatario)
{
	message_hdr_t * notifica = malloc(sizeof(message_hdr_t));
	
	setHeader(&(*notifica), tipoNotifica, "");

	connectionSocketLocksLock(FDSocketDestinatario, connectionSocketLocksChatty);			
	//invio notifica
	sendHeader(FDSocketDestinatario, notifica);
	//se è di tipo text o file invio
	
	connectionSocketLocksUnlock(FDSocketDestinatario, connectionSocketLocksChatty);
	
	free(notifica);
	return 1;
}



int inviaListaOnline(long FDSocketDestinatario)
{
	
	connectionSocketLocksLock(FDSocketDestinatario, connectionSocketLocksChatty);
	
	message_t * utentiOnline = malloc(sizeof(message_t));
	
	ParametriListaOnline * dettagliListaOnline = listaOnlineRitrovaParametri(listaUtentiOnlineChatty);
	
	char * buffer = dettagliListaOnline -> buf;
	int dimBuffer = dettagliListaOnline -> len;
			
	setHeader(&(*utentiOnline).hdr, OP_OK, "");
	setData(&(*utentiOnline).data, "", buffer, dimBuffer);
			
	//invio messaggio
	sendRequest(FDSocketDestinatario, utentiOnline);
	
	free(dettagliListaOnline -> buf);	//CRITICO	//GRIND
	free(dettagliListaOnline);
	free(utentiOnline);
	
	connectionSocketLocksUnlock(FDSocketDestinatario, connectionSocketLocksChatty);

	return 1;
}

int messageHandler(message_t * messaggio, long FDSocketClient)
{
	int tipoOP = messaggio -> hdr . op;
	
	//valorizzo qui le variabili del messaggio
	char * sender = malloc(MAX_NAME_LENGTH + 1 * sizeof(char));
	char * receiver = malloc(MAX_NAME_LENGTH + 1 * sizeof(char));
	
	strncpy(sender, messaggio -> hdr . sender, MAX_NAME_LENGTH + 1);			//cambiato con max name lenght
	strncpy(receiver, messaggio -> data . hdr . receiver,  MAX_NAME_LENGTH + 1);
	sender[MAX_NAME_LENGTH] = '\0';
	receiver[MAX_NAME_LENGTH] = '\0';
	
	unsigned int len = messaggio -> data . hdr . len;
	
	char * buf = malloc(sizeof(char) * len);
	strncpy(buf, messaggio -> data . buf, len);
	
	switch(tipoOP)
	{
		case REGISTER_OP:
		{
			//se il massimo delle connessioni è già stato raggiunto invio errore OP_MAX_CONNECTIONS
			int numeroOnline = listaOnlineRitrovaNumeroOnline(listaUtentiOnlineChatty);
			if (numeroOnline >= maxConnessioni)
			{inviaNotifica(OP_MAX_CONNECTIONS, FDSocketClient); chattyStats . nerrors ++;	break;}
			//se il nickname esiste già invio messaggio di errore OP_NICK_ALREADY
			if(dbUtentiCercaNickname(databaseUtentiChatty, sender) == 1)
			{inviaNotifica(OP_NICK_ALREADY, FDSocketClient); chattyStats . nerrors ++;	break;}
			//se il nickname non esiste lo aggiungo al database utenti e vado online
			dbUtentiRegistraNickname(databaseUtentiChatty, sender);
			listaOnlineNodoUtenteAggiungi(listaUtentiOnlineChatty, sender, FDSocketClient);
			//invia messaggio lista online
			inviaListaOnline(FDSocketClient);
		}break;
		case CONNECT_OP:
		{
			//se il massimo delle connessioni è già stato raggiunto invio errore OP_MAX_CONNECTIONS
			int numeroOnline = listaOnlineRitrovaNumeroOnline(listaUtentiOnlineChatty);
			if (numeroOnline >= maxConnessioni)
			{inviaNotifica(OP_MAX_CONNECTIONS, FDSocketClient); chattyStats . nerrors ++;	break;}
			//se il nickname non esistesse invio messaggio di errore OP_NICK_UNKNOWN
			if(dbUtentiCercaNickname(databaseUtentiChatty, sender) == 0)
			{inviaNotifica(OP_NICK_UNKNOWN, FDSocketClient); chattyStats . nerrors ++;	break;}
			//altrimenti lo aggiungo alla lista online e la invio
			listaOnlineNodoUtenteAggiungi(listaUtentiOnlineChatty, sender, FDSocketClient);
			inviaListaOnline(FDSocketClient);
		}break;
		case POSTTXT_OP:
		{
			//non posso mandare messaggi a me stesso, se fosse così errori
			if(strcmp(sender, receiver) == 0)
			{inviaNotifica(OP_FAIL, FDSocketClient); chattyStats . nerrors ++;	break;}
			//se il messaggio fosse troppo lungo invio notifica di errore OP_MSG_TOOLONG
			if(len > dimensioneMaxMessaggio)
			{inviaNotifica(OP_MSG_TOOLONG, FDSocketClient); chattyStats . nerrors ++;	break;}
			//se il nickname del destinatario non esistesse invio messaggio di errore OP_NICK_UNKNOWN
			if(dbUtentiCercaNickname(databaseUtentiChatty, receiver) == 0)
			{inviaNotifica(OP_NICK_UNKNOWN, FDSocketClient); chattyStats . nerrors ++;	break;}
			//altrimenti aggiungo alla history il messaggio
			dbUtentiInviaMessaggio(databaseUtentiChatty, TXT_MESSAGE, sender, receiver, buf, len);
			//se l'utente fosse online gli invio una notifica TXT_MESSAGE
			
			long FDSocketDestinatario = listaOnlineNodoUtenteOttieniSocket(listaUtentiOnlineChatty, receiver);
			if(FDSocketDestinatario != -1)
			{
				//printf("EHI! PARE CHE: %s SIA CONNESS* SULLA SOCKET: %li\n", receiver, FDSocketDestinatario);
				//pthread_mutex_lock(&setLock);	//blocco il set per evitare l'invio di messaggi a client che si sono disconnessi
			inviaNotificaMessaggio(TXT_MESSAGE, FDSocketDestinatario, & messaggio -> hdr, & messaggio -> data);
				//pthread_mutex_unlock(&setLock);
			}
			
			//altrimenti tutto ok
			inviaNotifica(OP_OK, FDSocketClient);
		}break;
		case POSTTXTALL_OP:
		{
			//se il messaggio fosse troppo lungo invio notifica di errore OP_MSG_TOOLONG
			if(len > dimensioneMaxMessaggio)
			{inviaNotifica(OP_MSG_TOOLONG, FDSocketClient); chattyStats . nerrors ++;	break;}
			//faccio il broadcast del messaggio
			dbUtentiBroadcastMessaggio(databaseUtentiChatty, TXT_MESSAGE, sender, buf, len);
			
			broadcastOnlineNotifica(TXT_MESSAGE ,listaUtentiOnlineChatty, FDSocketClient, messaggio);
			
			inviaNotifica(OP_OK, FDSocketClient);
		}break;
		case POSTFILE_OP:
		{
			printf("INVIO FILE");
			//il client mi invia prima un messaggio contenente l'intestazione
			//e poi un data con il file vero e proprio
			
			//char * nomeFile =  malloc(sizeof(char)*len);
			//strncpy(nomeFile, buf, len);	//con questo ho il nome del file
			
			char * nomeFile = buf;
			
			//devo sapere se il file esiste
			
			
			//ora faccio un readdata per leggere il file stesso
			/*message_data_t * dataFile = malloc(sizeof(message_data_t));
			int risFile = readData(FDSocketClient, dataFile);
			if (risFile < 0) {
				perror("readDataFile");
				break;
			}*/
			
			message_data_t dataFile;
			int risFile = readData(FDSocketClient, &dataFile);
			if (risFile < 0) {
				perror("readDataFile");
				break;}
				
			//devo essere sicuro che la sua dimensione sia entro i limiti
			int dimensioneFile = dataFile . hdr . len;
			void * file = dataFile .  buf;
			
			if(dimensioneFile > BYTE_TO_KILOBYTE *  dimensioneKByteMaxFile)	/*la dimensione massima è in kilobytes o bytes?*/
			{	inviaNotifica(OP_MSG_TOOLONG, FDSocketClient); chattyStats . nerrors ++; break;	}
			
			//creo una directory 
			int risDirecory = mkdir(pathFiles, MASCHERA_DIRITTI);
			if (risDirecory < 0)
			{
				if (errno != EEXIST)
				{perror("mkdir");		break;}
			}
			
			//creo un file nella directory files con il nome noto e lo apro in scrittura
			//con libgen ritrovo il filename
			nomeFile = basename(nomeFile);
			//concateno nome file e directory files del server
			char * pathCompletaFile = malloc(sizeof(char) * 1024);
			sprintf(pathCompletaFile, "%s/%s", pathFiles , nomeFile);
			
			int FDFileDaScrivere = open(pathCompletaFile, O_WRONLY | O_CREAT, MASCHERA_DIRITTI);
			if (FDFileDaScrivere < 0)
			{
				perror("open");		break;
			}
			//ci scrivo sopra il file
			int risWrite = write(FDFileDaScrivere, file, dimensioneFile);
			if (risWrite < 0)
			{
				perror("write");	break;
			}
			int risClose = close(FDFileDaScrivere);
			if (risClose < 0)
			{
				perror("close");	break;
			}
			//tutto a posto, aggiungo alla history ed invio ok
			dbUtentiInviaMessaggio(databaseUtentiChatty, FILE_MESSAGE, sender, receiver, buf, len);
			//se l'utente fosse online gli invio una notifica FILE_MESSAGE
			
			long FDSocketDestinatario = listaOnlineNodoUtenteOttieniSocket(listaUtentiOnlineChatty, receiver);
			if(FDSocketDestinatario != -1)
			{
				//printf("EHI! PARE CHE: %s SIA CONNESS* SULLA SOCKET: %li\n", receiver, FDSocketDestinatario);
				//pthread_mutex_lock(&setLock);	//blocco il set per evitare l'invio di messaggi a client che si sono disconnessi
				inviaNotificaMessaggio(FILE_MESSAGE, FDSocketDestinatario, & messaggio -> hdr, & messaggio -> data);
				//pthread_mutex_unlock(&setLock);
			}
			//invio ok
			
			
			free(pathCompletaFile); //CRITICO
			free(dataFile . buf);	//CRITICO	//GRIND
			inviaNotifica(OP_OK, FDSocketClient);

		}break;
		case GETFILE_OP:
		{
			
			//char * nomeFile;
			//nomeFile =  malloc(sizeof(char)*len);
			//strncpy(nomeFile, buf, len);	//con questo ho il nome del file
			//ricevo il nome del file
			char * nomeFile = buf;
			nomeFile = basename(nomeFile);
			//concateno nome file e directory files del server
			char * pathCompletaFile = malloc(sizeof(char) * 1024);
			sprintf(pathCompletaFile, "%s/%s", pathFiles , nomeFile);
			
			
			//lo cerco nella directory
			//devo concatenare directory e nome file
			int FDFileDaMappare = open(pathCompletaFile, O_RDONLY);
			//devo distinguere il caso fallimento dal caso file non esiste
			if (FDFileDaMappare < 0)
			{
				//se il file non esiste
				if (errno == ENOENT) {inviaNotifica(OP_NO_SUCH_FILE, FDSocketClient); chattyStats . nerrors ++; break;}
				else
				perror("open");		break;
			}
			//se esiste lo mappo e invio void* mappato e sua dimensione
			//uso le stat del file per saperne la dimensione
			
			struct stat statsFile;
			if (stat(pathCompletaFile, &statsFile)==-1) 
			{
				perror("stat");
				break;
			}
			if (!S_ISREG(statsFile.st_mode))
			{
				fprintf(stderr, "ERRORE: il file %s non e' un file regolare\n", pathCompletaFile);
				return -1;
			}
			int dimensioneFile = statsFile.st_size; // size del file 
			
			void * fileMappato = mmap(NULL, dimensioneFile , PROT_READ,MAP_PRIVATE, FDFileDaMappare, 0);
			
			message_t * fileMessaggio = malloc(sizeof(message_t));
			
			//ho tutto, posso impostare header, dati e inviare
			setHeader(&(*fileMessaggio).hdr, OP_OK, "");
			setData(&(*fileMessaggio).data, "", fileMappato, dimensioneFile);
			
			//tutto ok
			sendRequest(FDSocketClient, fileMessaggio);
			
			//unmap del file
			munmap(fileMappato, dimensioneFile);
			
			free(fileMessaggio);	//CRITICO	//GRIND
			free(pathCompletaFile); //CRITICO
			
			
		}break;
		case GETPREVMSGS_OP:
		{
			inviaMessaggi(FDSocketClient, sender);

		}break;
		case USRLIST_OP:
		{
			//ricevo la lista degli utenti online
			inviaListaOnline(FDSocketClient);
		}break;
		case UNREGISTER_OP:
		{
			//devo controllare che il client non stia cercando di deregistrare qualcun altro
			if(strncmp(sender, receiver, MAX_NAME_LENGTH+1) != 0)
			{inviaNotifica(OP_FAIL,  FDSocketClient);	chattyStats . nerrors ++; break;}
			//rimuovo l'utente dal database
			dbUtentiDeregistraNickname(databaseUtentiChatty, sender);
			//tutto ok
			inviaNotifica(OP_OK, FDSocketClient);
		}break;
		case DISCONNECT_OP:	//non viene gestito esplicitamente
		default:
		{
			inviaNotifica(OP_FAIL,  FDSocketClient);
			chattyStats . nerrors ++;
		}break;
	}
	
	free(sender);
	free(receiver);
	free(buf);
	
	return 1;
}


int gestioneClient(long FDSocketDaGestire) 
{
		message_t messaggio;
		messaggio .data . buf = NULL;
		//message_t * messaggio = malloc(sizeof(message_t));
		//ignoro la sigpipe per non terminare in caso di scrittura su socket chiusa
		signal(SIGPIPE, SIG_IGN);
		
		
		while(flagTerminazioneServer == 0)
		{
			int ris = 0;
			
			ris = readMsg(FDSocketDaGestire, &messaggio);

			if(ris > 0)	//se ris == 0 è una disconnessione
			{messageHandler(&messaggio, FDSocketDaGestire);
			free(messaggio . data . buf);}
			else 
			{free(messaggio . data . buf);
			break;	}
		}
		//printf("Disconnessione client FD%d", (int) FDSocketClient);
		
		listaOnlineNodoUtenteRimuovi(listaUtentiOnlineChatty, FDSocketDaGestire);	//ci siamo disconnessi, rimuoviamo
		
		//disconnessione, procedo a rimuoverlo dal set
		pthread_mutex_lock(&setLock);
		
			FD_CLR(FDSocketDaGestire, &setPrincipale);
			if(FDSocketDaGestire == fdMassimo)
			{
				for(int i=(fdMassimo-1);i>=0;--i)
				if (FD_ISSET(i, &setPrincipale)) { fdMassimo = i; break; }
				else fdMassimo = -1;
			}
			close(FDSocketDaGestire);
		pthread_mutex_unlock(&setLock);
		//printf("Disconnesso client FD%d", (int) FDSocketClient);

		return 0;
		
}
