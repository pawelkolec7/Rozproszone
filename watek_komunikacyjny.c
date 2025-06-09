#include "main.h"
#include "watek_komunikacyjny.h"

//REQUEST           Odpowiada ACK od razu lub dodaje do kolejki
//ACK               Dodaje nadawcę do grupy kandydatów
//REQUEST_PLACE     Odpowiada ACK_PLACE (zgoda na miejsce)
//ACK_PLACE	        Zlicza otrzymane potwierdzenia miejsca
//START_GAME	    Wchodzi w stan InSection — zaczyna grę
//SET_BUSY	        Anuluje próbę gry (jeśli był w InWant/InWantPlace)
//RELEASE	        Oznacza miejsce jako wolne

// Sprawdza, czy dany proces (src) należy do pierwszych trzech kandydatów do gry
static int isCandidate(int src) {
    for (int i = 0; i < 3; i++) {
        if (groupCandidates[i] == src) return 1;
    }
    return 0;
}

// Funkcja główna wątku komunikacyjnego - odbiera i obsługuje wiadomości
void *startKomWatek(void *ptr) {
    MPI_Status status;
    packet_t pakiet;

    while (1) {
        pthread_mutex_lock(&stateMut);
        // Jeśli nasz stan to "zakończ", kończymy wątek
        if (stan == InFinish) {
            pthread_mutex_unlock(&stateMut);
            break;
        }
        pthread_mutex_unlock(&stateMut);
        // Odbieramy wiadomość od dowolnego źródła, dowolnego typu
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        updateClock(pakiet.ts);

        //Obsługa przychodzących wiadomości zależnie od ich typu (tagu)
        switch (status.MPI_TAG) {

            // Otrzymano prośbę o możliwość gry
            case REQUEST: {
                pthread_mutex_lock(&stateMut);
                state_t me = stan;
                int myTs = lamportClock;
                pthread_mutex_unlock(&stateMut);

                int src = status.MPI_SOURCE;
                int srcTs = pakiet.ts;
                int sendAck = 0;

                // Jeśli nie ubiegamy się o grę, od razu ACK
                if (me != InWant) {
                    sendAck = 1;
                } else {
                    // Jeśli ubiegamy się, porównujemy zegary i rangi:
                    if (srcTs < myTs || (srcTs == myTs && src < rank)) {
                        sendAck = 1;
                    } else {
                        // Jeśli nie, odkładamy src do kolejki oczekujących
                        pthread_mutex_lock(&stateMut);
                        WaitQueue[WaitQueueSize++] = src;
                        pthread_mutex_unlock(&stateMut);
                        debug("Dodano %d do kolejki oczekujących", src);
                    }
                }
                // Wysyłamy potwierdzenie
                if (sendAck) {
                    sendPacket(NULL, src, ACK);
                }
                break;
            }

            // Otrzymano ACK na naszą prośbę o grę
            case ACK: {
                println("Odebrałem ACK_GAME od procesu %d", status.MPI_SOURCE);
                pthread_mutex_lock(&stateMut);
                // Dodajemy źródło do listy kandydatów
                groupCandidates[groupSize++] = status.MPI_SOURCE;
                pthread_mutex_unlock(&stateMut);
                break;
            }

            // Ktoś pyta nas o miejsce do gry
            case REQUEST_PLACE: {
                debug("Otrzymałem REQUEST_PLACE od %d", status.MPI_SOURCE);
                 // Od razu odpowiadamy ACK_PLACE
                sendPacket(NULL, status.MPI_SOURCE, ACK_PLACE);
                break;
            }

            // Otrzymano ACK na nasze pytanie o miejsce
            case ACK_PLACE: {
                debug("Dostałem ACK_PLACE od %d", status.MPI_SOURCE);
                // Sprawdzamy czy to nasz kandydat i zwiększamy licznik otrzymanych ACK_PLACE
                if (isCandidate(status.MPI_SOURCE)) {
                    pthread_mutex_lock(&stateMut);
                    ackPlaceCount++;
                    pthread_mutex_unlock(&stateMut);
                }
                break;
            }

            // Otrzymano START_GAME - zaczynamy grać
            case START_GAME: {
                println("Otrzymałem START_GAME od %d", status.MPI_SOURCE);

                pthread_mutex_lock(&stateMut);
                groupReady = 1;     // Oznaczamy grupę jako gotową
                placeBusy = 1;      // Oznaczamy miejsce jako zajęte
                stan = InSection;   // Bezwzględnie zmieniamy stan na "gramy"
                pthread_mutex_unlock(&stateMut);

                println("Wchodzę do sekcji krytycznej (po otrzymaniu START_GAME)");
                break;
            }

            // Inny proces zajął miejsce - musimy anulować próbę
            case SET_BUSY: {
                debug("Otrzymałem SET_BUSY od %d", status.MPI_SOURCE);
                pthread_mutex_lock(&stateMut);
                placeBusy = 1;        // Ustawiamy miejsce jako zajęte
                state_t prev = stan;  // Zapamiętujemy stan przed zmianą
                pthread_mutex_unlock(&stateMut);

                if (prev == InWant || prev == InWantPlace) {
                    println("Anuluję próbę - miejsce zajęte!");
                    changeState(InRun); // Wracamy do InRun (nie próbujemy grać)
                }
                break;
            }

            // Inny proces zakończył grę - miejsce jest wolne
            case RELEASE: {
                debug("Otrzymałem RELEASE od %d", status.MPI_SOURCE);
                pthread_mutex_lock(&stateMut);
                placeBusy = 0; // Oznaczamy miejsce jako wolne
                pthread_mutex_unlock(&stateMut);
                break;
            }

            // Jeśli nieznany typ wiadomości, nic nie robimy
            default:
                break;
        }
    }
    return NULL;
}
