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
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"

/* Variabili globali
 * dimensioneDizionario	- la dimensione del dizionario
 */
 
static int dimensioneDizionario;



/* Metodi */

/** @function parserDizionarioAggiungi
 * 
 * @brief crea e restituisce una nuova entry per il dizionario
 * 
 * @param nuovoNome			stringa nome della entry
 * @param nuovoDefinizione	stringa definizione della entry
 * 
 * @return	nuova entry del dizionario in caso di successo
 * 			NULL in caso di fallimento
 * 
 */

DizionarioEntry * parserDizionarioAggiungi(char * nuovoNome, char * nuovoDefinizione)	{
	DizionarioEntry * nuovaEntry;
	nuovaEntry = malloc(sizeof(DizionarioEntry));
	
	if ((strcmp(nuovoNome, "")!=0) || (strcmp(nuovoDefinizione, "")!=0))
	{
		
		nuovaEntry -> nome = malloc(sizeof(char)*(strlen(nuovoNome)+1));
		nuovaEntry -> definizione = malloc(sizeof(char)*(strlen(nuovoDefinizione)+1));
		
		strncpy(nuovaEntry -> nome, nuovoNome, strlen(nuovoNome)+1);
		strncpy(nuovaEntry -> definizione, nuovoDefinizione, strlen(nuovoDefinizione)+1);

		return nuovaEntry;
	}
	return NULL;
}

/** @function parserDizionarioTokenizzaDefinizione
 * 
 * @brief tokenizza una stringa estratta dal file di config se soddisfa certi requisiti
 * 
 * @param linea			stringa estratta dal file di config
 * @param nome			stringa associata al nome di una entry, rientrante
 * @param definizione	stringa associata alla definizione di una entry, rientrante
 * 
 * @return	1 se la linea soddisfa i requisiti e viene tokenizzata con successo
 * 			0 altrimenti
 * 
 */
 
int parserDizionarioTokenizzaDefinizione (char * linea, char * nome, char * definizione)	{
	

	char * testercommento=NULL;
	char * punttoken;
	char * token=NULL;
	
	/* Addendum: filtri e tokenizzazione delle linee
	 * Prima di tokenizzare effettivamente la stringa si effettuano due controlli:
	 * Il primo controllo salta (restituendo 0) stringhe che iniziano con il carattere '#' perchè identifica una linea che è un commento
	 * La prima operazione di tokenizzazione estrae la sotto stringa che precede il primo carattere '#' per eliminare i commenti interlinea
	 * A questo punto si può tokenizzare la sottostringa estraendo nome e definizione
	 * Nel caso in cui si tokenizzasse qualcosa di nullo la stringa non è valida a priori, e la si esclude dall'aggiunta al dizionario
	 */
																		
	char primocarattere=linea[0];
	if( primocarattere != '#')	{
																		
		token=strtok_r(linea, "#", &testercommento);
		if (token == NULL) return 0;
		//strcpy(linea,token);	//CRITICO	//ERROR
		memmove(linea, token, strlen(linea));	//per evitare errore di overlap di memoria
		
		
		token=strtok_r(linea, " =\t\n", &punttoken);
		if (token == NULL) return 0;
		strcpy(nome, token);
																	
		token=strtok_r(NULL, " =\t\n", &punttoken);
		strcpy(definizione, token);
		
		return 1;
		}
	return 0;
	}

/** @function parserDizionarioInit
 * 
 * @brief costruisce un dizionario a partire dal path di un file di config
 * 
 * @param config	path del file di configurazione
 * 
 * @return	nuovo DizionarioCompilato se ha avuto successo
 * 			NULL altrimenti
 * 
 */
 
DizionarioCompilato * parserDizionarioInit(char * config)	{	

	FILE * configDaAprire;
	
	char * linea = NULL;
	size_t dimbuffer=0;
	ssize_t dimlinea=0;
	
	char * nomeDizionario;
	char * definizioneDizionario;
	dimensioneDizionario = DIM_INIZIALE_DIZIONARIO;
	
	DizionarioCompilato * dizionario;
	int indiceDizionario;
	
																		//Apro il file e alloco il dizionario
	configDaAprire = fopen(config,"r");
	if (!configDaAprire) {fprintf(stderr, "Errore nell'apertura del file di Config \n"); return NULL;}
	
	dizionario = malloc(sizeof(DizionarioCompilato));
	if (!dizionario) {fprintf(stderr, "Errore nell'allocazione del dizionario \n"); return NULL;}
	
																		
	dizionario -> entry = malloc(sizeof(DizionarioEntry)*DIM_INIZIALE_DIZIONARIO);
	if (!dizionario -> entry) {fprintf(stderr, "Errore nell'allocazione dello spazio delle definizioni del dizionario \n"); return NULL;}
	
	nomeDizionario = malloc(DIM_MASSIMA_ENTRY*sizeof(char));
	definizioneDizionario = malloc(DIM_MASSIMA_ENTRY*sizeof(char));
	if((!nomeDizionario)||(!definizioneDizionario)){fprintf(stderr, "Errore nell'allocazione delle stringhe delle definizioni del dizionario \n"); return NULL;}
	
	indiceDizionario = 0;
	
	//Scansiono il file linea per linea
	dimlinea = getline(&linea, &dimbuffer, configDaAprire);
	while (dimlinea != -1)
	{	
		if (dimlinea != 1) 												//Se dimlinea == 1 allora linea nulla
		{
			int accettata;
			if ((accettata=parserDizionarioTokenizzaDefinizione(linea, nomeDizionario, definizioneDizionario))==1 )
			{
				if(indiceDizionario>=DIM_INIZIALE_DIZIONARIO)
				{
					 dizionario -> entry = realloc(dizionario ->  entry, sizeof(DizionarioEntry)*(indiceDizionario+1));
					 if (!dizionario -> entry) {fprintf(stderr, "Errore nella riallocazione dello spazio delle definizioni del dizionario \n"); return NULL;}
					 dimensioneDizionario ++;
				}
				
				if((dizionario -> entry[indiceDizionario] = parserDizionarioAggiungi(nomeDizionario, definizioneDizionario))==NULL)
				{fprintf(stderr, "Errore nell'aggiunta della definizione al dizionario \n"); return NULL;}
				indiceDizionario ++;
			}
		}
		dimlinea = getline(&linea, &dimbuffer, configDaAprire);
	}
	
	free(linea);
	free(nomeDizionario);
	free(definizioneDizionario);
	
	fclose(configDaAprire);
	

	
	return dizionario;
}

/** @function parserDizionarioLookupDefinizione
 * 
 * @brief effettua una ricerca lineare sul dizionario per nome
 * 
 * @param nome			stringa nome da cercare
 * @param dizionario	DizionarioCompilato su cui effettuare la ricerca
 * 
 * @return	stringa definizione se ha avuto successo
 * 			NULL altrimenti
 * 
 */

char * parserDizionarioLookupDefinizione(char * nome, DizionarioCompilato * dizionario)	{
	
	for(int i=0; i<dimensioneDizionario; i++)
	{
		if(strcmp(nome, dizionario -> entry[i] -> nome)==0)
		return dizionario -> entry[i] -> definizione;
	}
	fprintf(stderr, "Errore nel lookup del dizionario \n");
	return NULL;
}

/** @function parserDizionarioTermina
 * 
 * @brief dealloca il dizionario
 * 
 * @param dizionario	dizionario da deallocare
 * 
 * @return	1 se ha avuto successo
 * 			0 altrimenti
 * 
 */
int parserDizionarioTermina(DizionarioCompilato * dizionario)
{
	if(dizionario == NULL) {fprintf(stderr, "Errore di accesso al dizionario per la terminazione\n"); return 0;}
																	
	//ora posso deallocare il dizionario
	//itero su ogni elemento del dizionario, dealloco nome, definizione
	//e poi dealloco il nodo
	for(int i=0; i<dimensioneDizionario; i++)
	{
		if(dizionario -> entry[i] -> nome == NULL) {fprintf(stderr, "Errore di accesso al nome di una entry del dizionario per la terminazione\n"); return 0;}
		free(dizionario -> entry[i] -> nome);
		if(dizionario -> entry[i] -> definizione == NULL) {fprintf(stderr, "Errore di accesso alla definizione di una entry dizionario per la terminazione\n"); return 0;}
		free(dizionario -> entry[i] -> definizione);
		if(dizionario -> entry[i]  == NULL) {fprintf(stderr, "Errore di accesso a una entry del dizionario per la terminazione\n"); return 0;}
		free(dizionario -> entry[i]);
	}
	if(dizionario -> entry  == NULL) {fprintf(stderr, "Errore di accesso a una entry dizionario per la terminazione\n"); return 0;}
	free(dizionario -> entry);
	free(dizionario);
	return 1;
}
