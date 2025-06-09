#include "main.h"
#include "watek_glowny.h"

void mainLoop() {
    srandom(rank);

    while (1) {
        // pobranie aktualnego stanu
        pthread_mutex_lock(&stateMut);
        state_t current = stan;
        pthread_mutex_unlock(&stateMut);

        // Jeśli proces ma zakończyć działanie, wychodzimy z pętli
        if (current == InFinish) break;

        switch (current) {
            // Proces "biega" - jeszcze nie chce grać
            case InRun: {
                int perc = random() % 100;
                // Z prawdopodobieństwem 10%
                if (perc < STATE_CHANGE_PROB) {
                    pthread_mutex_lock(&stateMut); // żądamy gry
                    lamportClock++;  // Zwiększamy zegar Lamporta (nowe zdarzenie lokalne)
                    println("Ubiegam się o grę w karty");
                    packet_t pkt = { .ts = lamportClock, .data = 0 };
                    groupSize = 0;
                    ackPlaceCount = 0;
                    pthread_mutex_unlock(&stateMut);
                    // Wysyłamy REQUEST do wszystkich innych procesów
                    for (int i = 0; i < size; i++)
                        if (i != rank)
                            sendPacket(&pkt, i, REQUEST);
                    // Zmieniamy stan na InWant - chcemy grać
                    changeState(InWant);
                }
                break;
            }

            case InWant: {
                // Czekamy na chętnych do gry
                // gdy uzbieramy ≥3 chętnych, prosimy o miejsce
                pthread_mutex_lock(&stateMut);
                int got = groupSize;    // Ilu kandydatów mamy?
                int busy = placeBusy;   // Czy miejsce już zajęte?
                int ready = groupReady; // Czy grupa gotowa?
                pthread_mutex_unlock(&stateMut);

                // Jeśli mamy ≥3 kandydatów i miejsce wolne i jeszcze nie jesteśmy gotowi
                if (got >= 3 && !ready && !busy) {
                    pthread_mutex_lock(&stateMut);
                    println("Zebrano grupę, ubiegam się o miejsce!");
                    groupReady = 1;     // Oznaczamy, że grupa jest gotowa
                    lamportClock++;     // Nowe lokalne zdarzenie
                    packet_t pkt = { .ts = lamportClock, .data = 0 };
                    ackPlaceCount = 0;
                    pthread_mutex_unlock(&stateMut);

                    // Wysyłamy REQUEST_PLACE tylko do 3 wybranych kandydatów
                    for (int i = 0; i < 3; i++) {
                        int dest = groupCandidates[i];
                        sendPacket(&pkt, dest, REQUEST_PLACE);
                    }
                    changeState(InWantPlace);
                }
                break;
            }

            // Czekamy na potwierdzenia miejsca
            case InWantPlace: {
                pthread_mutex_lock(&stateMut);
                int gotAck = ackPlaceCount;
                pthread_mutex_unlock(&stateMut);

                // Jeśli mamy 3 potwierdzenia
                if (gotAck >= 3) {
                    pthread_mutex_lock(&stateMut);
                    placeBusy = 1; // Zajmujemy miejsce
                    pthread_mutex_unlock(&stateMut);

                    // Najpierw blokujemy wszystkich
                    for (int i = 0; i < size; i++)
                        if (i != rank)
                            sendPacket(NULL, i, SET_BUSY);

                    //Potem krótki sleep, żeby mieć pewność, że SET_BUSY się rozpropagował
                    usleep(50000);

                    println("Miejsce wolne! Rozpoczynamy grę!");

                    //Dopiero teraz START_GAME do kandydatów
                    for (int i = 0; i < 3; i++)
                        sendPacket(NULL, groupCandidates[i], START_GAME);
                    // Wchodzimy do sekcji krytycznej (gramy w karty)
                    changeState(InSection);
                }
                break;
            }

            // Proces znajduje się w sekcji krytycznej (gra)
            case InSection: {
                println("Jestem w sekcji krytycznej (gram w karty)");
                sleep(10);
                println("Koniec gry, wychodze z sekcji krytycznej");

                 // Wysyłamy RELEASE do wszystkich, żeby poinformować, że zwalniamy miejsce
                packet_t pkt = { .data = 0 };
                for (int i = 0; i < size; i++)
                    if (i != rank)
                        sendPacket(&pkt, i, RELEASE);

                //Obsługa kolejki oczekujących
                pthread_mutex_lock(&stateMut);
                for (int i = 0; i < WaitQueueSize; i++) {
                    // Wysyłamy ACK wszystkim oczekującym
                    sendPacket(NULL, WaitQueue[i], ACK);
                }
                WaitQueueSize = 0;
                pthread_mutex_unlock(&stateMut);

                // Resetujemy stan procesu
                pthread_mutex_lock(&stateMut);
                placeBusy  = 0;
                groupSize  = 0;
                groupReady = 0;
                pthread_mutex_unlock(&stateMut);

                changeState(InRun);
                break;
            }
            default:
                sleep(1);
        }
        sleep(2);
    }
}
