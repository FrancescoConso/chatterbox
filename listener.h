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

#ifndef HANDLERCLIENT_H_
#define HANDLERCLIENT_H_
#include "message.h"
//#include "connections.h"
//#include "stats.h"

//#include "globals.h"

//static long FDSocketClient;//potrebbe causare problemi di sincronizzazione

int inviaMessaggi(long FDSocketDestinatario, char * sender);

int inviaNotifica(int tipoNotifica, int FDSocketDestinatario);

int inviaListaOnline(long FDSocketDestinatario);

int messageHandler(message_t * messaggio, long FDSocketClient);

int gestioneClient(long FDSocketDaGestire) ;
#endif
