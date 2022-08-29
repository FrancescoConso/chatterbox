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

#ifndef PARSER_H_
#define PARSER_H_
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Define globali:
 * DIM_MASSIMA_ENTRY		 - La dimensione massima (in caratteri) di ciascuna entry del dizionario associato al parser
 * DIM_INIZIALE_DIZIONARIO	 - Il dizionario viene inizializzato con questa dimensione, la dimensione è data dal numero di opzioni presenti nei file di configurazione
 */
 
#define DIM_MASSIMA_ENTRY 256
#define DIM_INIZIALE_DIZIONARIO 8


/* Strutture dati:
 * DizionarioCompilato	 - Il dizionario finale che viene generato dal parser, su cui il server farà il lookup al momento dell'inizializzazione, rappresentato come array di DizionarioEntry
 * DizionarioEntry		 - Entry del dizionario, composta da coppia nome - definizione
 */

typedef struct DizionarioEntry {
	char * nome;					//nome campo
	char * definizione;				//valore associato al campo
} DizionarioEntry;

typedef struct DizionarioCompilato {
	DizionarioEntry ** entry;		//singola entry del dizionario
} DizionarioCompilato;

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

DizionarioEntry * parserDizionarioAggiungi(char * nuovoNome, char * nuovoDefinizione);	

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
 
int parserDizionarioTokenizzaDefinizione (char * linea, char * nome, char * definizione);	

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
 
DizionarioCompilato * parserDizionarioInit(char * config);	

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

char * parserDizionarioLookupDefinizione(char * nome, DizionarioCompilato * dizionario);	

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

int parserDizionarioTermina(DizionarioCompilato * dizionario);

#endif
