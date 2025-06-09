#include "main.h"
#include "util.h"

MPI_Datatype MPI_PAKIET_T;
state_t stan = InRun;                                 // Aktualny stan procesu (np. InRun, InWant, InSection, InFinish)
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER; // Muteks do synchronizacji dostępu do stanu procesu i zegara

// Struktura pomocnicza do mapowania tagów MPI na ich czytelne nazwy
struct tagNames_t {
    const char *name;
    int tag;
} tagNames[] = {
    { "pakiet aplikacyjny", APP_PKT },
    { "finish", FINISH },
    { "potwierdzenie", ACK },
    { "prośbę o sekcję krytyczną", REQUEST },
    { "zwolnienie sekcji krytycznej", RELEASE },
    { "start gry", START_GAME }
};

// Funkcja: tłumaczy numer tagu na jego czytelną nazwę tekstową
const char *const tag2string(int tag) {
    for (int i = 0; i < sizeof(tagNames)/sizeof(struct tagNames_t); i++) {
        if (tagNames[i].tag == tag) return tagNames[i].name;
    }
    return "<unknown>";
}

// Funkcja: inicjalizuje własny typ MPI_PAKIET_T dla struktury packet_t
void inicjuj_typ_pakietu() {
    int blocklengths[NITEMS] = {1, 1, 1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);
}

// Funkcja: wysyła pakiet typu packet_t do innego procesu
void sendPacket(packet_t *pkt, int destination, int tag) {
    int freepkt = 0;
    if (pkt == NULL) {
        pkt = malloc(sizeof(packet_t));
        pkt->data = 0;
        freepkt = 1;
    }
    pkt->src = rank;
    pthread_mutex_lock(&stateMut);
    pkt->ts = lamportClock;
    pthread_mutex_unlock(&stateMut);

    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debug("Wysyłam %s do %d", tag2string(tag), destination);

    if (freepkt) free(pkt);
}

// Funkcja: zmienia stan procesu
void changeState(state_t newState) {
    pthread_mutex_lock(&stateMut);
    if (stan == InFinish) {
        pthread_mutex_unlock(&stateMut);
        return;
    }
    stan = newState;
    pthread_mutex_unlock(&stateMut);
}

// Funkcja: zwiększa zegar Lamporta o 1
void incrementClock() {
    pthread_mutex_lock(&stateMut);
    lamportClock++;
    pthread_mutex_unlock(&stateMut);
}

// Funkcja: aktualizuje zegar Lamporta na podstawie otrzymanego czasu
void updateClock(int received_ts) {
    pthread_mutex_lock(&stateMut);
    // Ustawiamy zegar na maksymalny (nasz lub otrzymany)
    if (lamportClock < received_ts)
        lamportClock = received_ts;
    // Zawsze zwiększamy zegar o 1 po otrzymaniu wiadomości
    lamportClock++;
    pthread_mutex_unlock(&stateMut);
}
