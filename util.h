#ifndef UTILH
#define UTILH
#include "main.h"

// Struktura reprezentująca pakiet przesyłany między procesami MPI
typedef struct {
    int ts;         // Zegar Lamporta - kiedy wiadomość została wysłana
    int src;        // Numer rank nadawcy wiadomości
    int data;       // Pole danych - używane np. do dodatkowych informacji
} packet_t;

#define NITEMS          3 // Liczba elementów w strukturze packet_t
#define ACK             1 // Potwierdzenie żądania gry
#define REQUEST         2 // Prośba o możliwość zagrania (do innych procesów)
#define RELEASE         3 // Zwolnienie sekcji krytycznej (kończymy grę)
#define APP_PKT         4 // Pakiet aplikacyjny (ogólny, niewykorzystywany w głównym kodzie)
#define FINISH          5 // Informacja o zakończeniu pracy procesu
#define START_GAME      6 // Rozpoczęcie gry (wszyscy kandydaci mogą wejść do gry)
#define REQUEST_PLACE   7 // Prośba o miejsce do gry (po zebraniu kandydatów)
#define ACK_PLACE       8 // Potwierdzenie przyznania miejsca
#define SET_BUSY        9 // Wiadomość informująca, że miejsce do gry zostało zajęte

extern MPI_Datatype MPI_PAKIET_T;
void inicjuj_typ_pakietu();
void sendPacket(packet_t *pkt, int destination, int tag);

// Typ wyliczeniowy reprezentujący aktualny stan procesu
typedef enum {
    InRun,        // Proces "biega luzem", nie chce jeszcze grać
    InWant,       // Proces ubiega się o możliwość gry (wysyła REQUEST)
    InWantPlace,  // Proces szuka miejsca (po zebraniu grupy)
    InSection,    // Proces jest w sekcji krytycznej (gra w karty)
    InFinish      // Proces kończy pracę
} state_t;

extern state_t stan;
extern pthread_mutex_t stateMut;
void changeState(state_t);
void incrementClock();
void updateClock(int received_ts);

#endif
