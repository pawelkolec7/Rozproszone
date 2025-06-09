#ifndef MAINH
#define MAINH
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "util.h"

#define TRUE 1
#define FALSE 0
#define SEC_IN_STATE 2       // Liczba sekund, jaką proces spędza w jednym stanie
#define STATE_CHANGE_PROB 10 // Prawdopodobieństwo zmiany stanu na "chcę grać" wynosi 10%
#define ROOT 0               // Numer procesu głównego (zazwyczaj rank == 0)
#define MAX_QUEUE 100        // Maksymalny rozmiar kolejki oczekujących procesów

extern int rank;
extern int size;
extern int lamportClock;
extern int ackCount;
extern int ackPlaceCount;
extern int groupCandidates[100];
extern int groupSize;
extern int groupReady;
extern int groupID;
extern int placeBusy;
extern pthread_t threadKom;
extern int WaitQueue[MAX_QUEUE];
extern int WaitQueueSize;

// Makro debugujące: wyświetla kolorowe wiadomości tylko, jeśli włączone DEBUG
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d][t%d]: " FORMAT "%c[%d;%dm\n", 27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

// Makro println: zawsze wyświetla wiadomość na ekranie z informacją o ranku i zegarze
#define println(FORMAT,...) printf("%c[%d;%dm [%d][t%d]: " FORMAT "%c[%d;%dm\n", 27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, ##__VA_ARGS__, 27,0,37);
#endif
