/*
 * Progetto Corso Sistemi Operativi e Laboratorio 2018
 * 
 * Francesco Consonni
 * francescoconsoconsonni@gmail.com
 * Matricola # 531373
 * 
 * Si dichiara che il programma è opera originale dello studente
 * ad eccezione dei metodi readn e writen, forniti dalle dispense
 * del corso di studio.
 * 
 */
 
 /**
 * Il modulo connections del server gestisce le connessioni dei client verso
 * il thread listener
 * 
 * @file connections.c
 * @brief Modulo connessioni del server
 *
 */
 
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>


#include <connections.h>


/** @function readn
 * 	
 * @brief legge size byte sulla socket individuata da fd sul buffer buf
 * 
 * @param fd	file descriptor della socket
 * @param buf	il buffer su cui saranno letti i dati
 * @param size	la dimensione dei byte da leggere
 * 
 * @return size in caso di successo
 * 			0 in caso di chiusura socket
 * 			errno altrimenti
 */

static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) {/*printf("Socket chiusa\n");*/	return 0; }  // gestione chiusura socket
        left    -= r;
	bufptr  += r;
    }
    //printf("Letti %d byte\n", size);
    return size;
}

/** @function writen
 * 	
 * @brief scrive size byte sulla socket individuata da fd dal buffer buf
 * 
 * @param fd	file descriptor della socket
 * @param buf	il buffer su cui saranno letti i dati
 * @param size	la dimensione dei byte da leggere
 * 
 * @return 1 in caso di successo
 * 			-1 altrimenti
 */


static inline int writen(long fd, void *buf, size_t size) {
	
		//int bytescritti=0;//TEST
	
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;	//il garbage potrebbe venire da qui
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
		//bytescritti=r;//TEST
    }
    //printf("Scritti %d byte\n", bytescritti);
    return 1;
}


/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server 
 *
 * @param path Path del socket AF_UNIX 
 * @param ntimes numero massimo di tentativi di risry
 * @param secs tempo di attesa tra due risry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
	//controllo che i parametri siano entro i requisiti della define
	if(strlen(path) > UNIX_PATH_MAX)	{perror("Path troppo lungo\n"); return -1;}
	if(ntimes > MAX_RETRIES)	{perror("Numero di retries troppo alto\n"); return -1;}
	if(secs > MAX_SLEEPING)		{perror("Tempo di attesa troppo lungo\n"); return -1;}

	
	
	struct sockaddr_un indirizzoClient;
	long FDSocketClient;
	
	int ris;
	
	do{
		//creo il socket del client
		FDSocketClient = socket(AF_UNIX, SOCK_STREAM, 0);
		if (FDSocketClient == -1)	{
				perror("socket");
				return -1;}
					
		//valorizzo famiglia e nome socket
		indirizzoClient.sun_family = AF_UNIX;
		strncpy(indirizzoClient.sun_path, path, strlen(path)+1);
		
		//associo il socket all'indirizzo (ho tot tentativi a disposizione)
		for(int i=0; i< ntimes; i++)
		{
			ris = connect(FDSocketClient, (struct sockaddr*) &indirizzoClient, sizeof(indirizzoClient));
			if (ris == -1)	{
			perror("connect");
			sleep(secs);
			}
			else break;
		}
	}
	while(0);
	if (ris == -1)	return -1;
	
	return FDSocketClient;
}


// -------- server side ----- 
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore 
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readHeader(long connfd, message_hdr_t *hdr)
{
	int ris = -1;
	do{
		ris = readn(connfd, hdr, sizeof(message_hdr_t));
		if (ris < 0) {
			perror("read");
			return -1;
			}
	}
	while(0);
	return ris;
}

/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readData(long fd, message_data_t *data)
{
	int risData = -1;
	int risBuffer = -1;
	do {
		//Leggo i metadati data
		
		risData = readn(fd, data, sizeof(message_data_t));
		if (risData < 0) {
			perror("read");
			return -1;
			}
		
		//Leggo il buffer
		
		unsigned int dimensioneMessaggio = data -> hdr . len;
		
		
		char * bufferData = malloc(sizeof(char) * dimensioneMessaggio);	//se il buffer è zero?
		
		risBuffer = readBuffer(fd, bufferData, dimensioneMessaggio);
		if(risBuffer < 0){
			perror("read");
			return -1;
			}
		
		data -> buf = bufferData;

	} while(0);
	
	return risData+risBuffer;
}

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readMsg(long fd, message_t *msg)
{
	int risHeader = -1;
	int risDataBuf = -1;
	
	
	
	do {
		//Leggo l'header
		message_hdr_t headerMessaggio;
		
		memset(&headerMessaggio, 0, sizeof(message_hdr_t));	//CRITICO	//GRIND	//ERROR
		
		risHeader = readn(fd, &headerMessaggio, sizeof(message_hdr_t));
		if(risHeader < 0){
			perror("read");
			return -1;
			}

		
		message_data_t headerData;
		
		memset(&headerData, 0, sizeof(message_data_t));	//CRITICO	//GRIND	//ERROR
		
		risDataBuf = readData(fd, &headerData);
		if(risDataBuf < 0){
			perror("read");
			return -1;
			}

		//assegno tutti i valori necessari
		msg -> hdr = headerMessaggio;
		msg -> data = headerData;
	}
	while (0);
	return risHeader+risDataBuf;
}

/* da completare da parte dello studente con altri metodi di interfaccia */

/**
 * @function readBuffer
 * @brief Legge il buffer del messaggio
 *
 * @param fd     descrittore della connessione
 * @param buffer   puntatore al buffer del messaggio
 * @param len 	lunghezza del buffer del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readBuffer(long fd, char * buffer, unsigned int len)
{
	//Leggo il buffer
	int ris = -1;
	do{
		ris = readn(fd, buffer, len);
		if (ris < 0) {
			perror("read");
			return -1;
			}
	}
	while (0);
		return ris;
}

// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg)
{
	int ris = -1;
	
	do{
		//Scrivere header
		//message_hdr_t headerMessaggio =  msg -> hdr;
		ris = writen(fd, msg, sizeof(message_hdr_t));
		if(ris < 0) {
		perror("write");
		return -1;
		}

		//Scrivere data
		message_data_t dataMessaggio = msg -> data;
		ris = sendData(fd , &dataMessaggio);
		
	}	while(0);

	return ris;
}

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg)
{
	int risData = -1;
	int risBuffer = -1;
	
	do{
		//Invio Metadati
		
		risData = writen(fd, msg, sizeof(message_data_t));
		if(risData < 0) {
		perror("write");
		return -1;
		}
		
		//Invio Buffer
		
		unsigned int dimensioneMessaggio = msg -> hdr . len;
		char * bufferDataDaCopiare = msg -> buf;
		
		char * bufferData = malloc(sizeof(char) * dimensioneMessaggio);
		
		//copio la stringa nella sua interezza con memcpy
		memcpy(bufferData, bufferDataDaCopiare, dimensioneMessaggio);
		
		//unsigned int dimensioneMessaggio = msg -> hdr . len;
		
		//char * bufferData = malloc(sizeof(char) * dimensioneMessaggio);
		
		//bufferData = msg -> buf;
		
		risBuffer = sendBuffer(fd, bufferData, dimensioneMessaggio);
		if(risBuffer < 0){
		perror("read");
		return -1;
		}
		
		
		free(bufferData);
		
		
	}	while(0);
	
	return risData+risBuffer;
}


/* da completare da parte dello studente con eventuali altri metodi di interfaccia */

/**
 * @function sendBuffer
 * @brief Invia il buffer del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param buffer    puntatore al buffer da inviare
 * @param len	lunghezza del buffer
 *
 * @return <=0 se c'e' stato un errore
 */
int sendBuffer(long fd, char * buffer, unsigned int len)
{
	int ris = -1;
	do{
		//Invio Buffer
		
		ris = writen(fd, buffer, len);
		if(ris < 0) {
		perror("write");
		return -1;
		}

	}	while(0);
	
	return ris;
}

/**
 * @function sendHeader
 * @brief Invia un header di messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendHeader(long fd, message_hdr_t *msg)
{
	int ris = -1;
	do{
		//Scrivere header
		message_hdr_t headerMessaggio =  * msg;
		ris = writen(fd, &headerMessaggio, sizeof(message_hdr_t));
		
		if(ris < 0) {
		perror("write");
		return -1;
		}
		
	}	while(0);

	return ris;
}
