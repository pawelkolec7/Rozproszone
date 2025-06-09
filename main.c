#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"

int rank, size;                  // rank - numer procesu MPI, size - liczba wszystkich procesów
int lamportClock = 0;            // zegar Lamporta dla porządkowania zdarzeń
int ackCount = 0;                // liczba otrzymanych ACK (potwierdzeń gry)
int ackPlaceCount = 0;           // liczba otrzymanych ACK_PLACE (potwierdzeń miejsca do gry)
int groupCandidates[100];        // lista kandydatów do gry
int groupSize = 0;               // liczba kandydatów
int groupReady = 0;              // flaga: czy grupa jest gotowa do gry
int groupID = 0;                 // (obecnie nieużywane) ID grupy
int placeBusy = 0;               // flaga: czy miejsce do gry jest zajęte
pthread_t threadKom;             // uchwyt do wątku komunikacyjnego
int WaitQueue[MAX_QUEUE];        // kolejka procesów czekających na ACK
int WaitQueueSize = 0;           // liczba elementów w kolejce oczekujących

// Funkcja kończąca działanie programu
void finalizuj()
{
    pthread_mutex_destroy(&stateMut);
    println("czekam na wątek \"komunikacyjny\"\n");
    pthread_join(threadKom, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

// Funkcja sprawdzająca jakie wsparcie dla wątków dostaliśmy od MPI
void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE:
            printf("Brak wsparcia dla wątków, kończę\n");
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
        case MPI_THREAD_FUNNELED:
            printf("tylko te wątki, które wykonały mpi_init_thread mogą wywoływać MPI\n");
        break;
        case MPI_THREAD_SERIALIZED:
            printf("tylko jeden wątek naraz może wywoływać MPI\n");
        break;
        case MPI_THREAD_MULTIPLE:
            printf("Pełne wsparcie dla wątków\n");
        break;
        default:
            printf("Nikt nic nie wie\n");
    }
}

int main(int argc, char **argv)
{
    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    inicjuj_typ_pakietu();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    pthread_create(&threadKom, NULL, startKomWatek, 0);

    mainLoop();

    finalizuj();
    return 0;
}
