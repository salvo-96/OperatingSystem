
#include "header.h"
typedef struct {
    long mtype; // Per come filtri il messaggio all'interno della coda
    char mtext[DIM_MSG]; // Il messaggio vero e proprio
} msg;
int main(void){

    msg messaggio;

    int pid_new_taxi;

    double idle_time = 0; // Contatore tempo di vita - Per i processi Taxi liberi ma inutilizzati
    double block_time = 0; // Contatore tempo condizione bloccato su una cella occupta da max_taxi

    int pid_richiesta;
    int x_richiesta;
    int y_richiesta;
    int x_taxi;
    int y_taxi;
    int x_destinazione;
    int y_destinazione;

    struct timeval start_taxi_time, end_taxi_time;

    double time_taken_taxi;

    int index;

    int time_request = 0;

    coordinate coordinate_taxi;

    int status = 0;

    queue = msgget(QUEUE_KEY, 0); //coda invio richieste

    shm = shmget(SHARED_MEMORY_KEY, sizeof(map), 0666);

    map *pointer_map_sh;

    request = semget(SEM_REQUEST_KEY, 1, 0);

    // sem_wait_childs = semget(SEM_WAIT_CHILDS_KEY, 1, 0);

    sem_mem_sh_taxi = semget(SEM_SH_TAXI_KEY, 1, 0);

    sem_simulation = semget(SEM_SIMULATION_KEY, 1, 0);

    sem_final = semget(SEM_FINAL_KEY, 1, 0);

    sem_sinc = semget(SEM_SINC_KEY, 1,0);    // sem_sinc: semaforo che permette agli studenti di cominicare tra loro;

    sem_taxi = semget(SEM_TAXI_KEY, 1, 0);

    while (semctl(sem_sinc, 0, GETVAL) != 1 );

    reserveSem(sem_sinc);//Prende per se il semaforo

    releaseSem(sem_final);

    pointer_map_sh = (map *) shmat(shm, NULL, 0);    //Si attacca alla memoria condivisa

    index = get_index_expired_taxi(pointer_map_sh);

    coordinate_taxi = estrai_casualmente_taxi_cell(pointer_map_sh);

    while (coordinate_taxi.x<0 && coordinate_taxi.y < 0){

        coordinate_taxi = estrai_casualmente_taxi_cell(pointer_map_sh);

    }

    pointer_map_sh->taxi_array[index].expired = 0;
    pointer_map_sh->taxi_array[index].targa = getpid();
    pointer_map_sh->taxi_array[index].disponibile = 1;
    pointer_map_sh->taxi_array[index].request_counter = 0;
    pointer_map_sh->taxi_array[index].posizione_attuale.x = coordinate_taxi.x;
    pointer_map_sh->taxi_array[index].posizione_attuale.y = coordinate_taxi.y;
    pointer_map_sh->taxi_array[index].distance_counter = 0;
    pointer_map_sh->taxi_array[index].bloccato = 0;


    pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti++;

    pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';


    shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa

    releaseSem(sem_sinc);//Rilascia il semaforo della memoria condivisa

    while (semctl(sem_simulation, 0, GETVAL) == 1 ) { // AVVIO COMMUNICATION DEI PROCESSI TAXI - POSSONO INIZIARE A SODDISFARE LE RICHIESTE !!

        // -------------   -------------   -------------   -------------   -------------  -------------  -------------  -------------
        signal(SIGINT, SIG_IGN);

        gettimeofday(&start_taxi_time, NULL);

        while (semctl(sem_mem_sh_taxi, 0, GETVAL) != 1 && semctl(sem_simulation,0,GETVAL) != 1);

        reserveSem(sem_mem_sh_taxi);

        pointer_map_sh = (map *) shmat(shm, NULL, 0);    //Si attacca alla memoria condivisa

        x_taxi = pointer_map_sh->taxi_array[index].posizione_attuale.x;
        y_taxi = pointer_map_sh->taxi_array[index].posizione_attuale.y;

        if (msgrcv(queue, &messaggio, sizeof(messaggio), getpid(), IPC_NOWAIT) != -1) { //Se c'è un messaggio

            if (sscanf(messaggio.mtext, "%d %d %d %d %d", &pid_richiesta, &x_richiesta,&y_richiesta,&x_destinazione, &y_destinazione) == 5) {

                printf(BOLDRED"\nSono il processo new_taxi : %d ---- RICHIESTA Trovata - PID Processo Richiesta : %d  - CellaRichiesta [%d-%d]  - Destinazione [%d-%d]\n"RESET,getpid(), pid_richiesta, x_richiesta, y_richiesta, x_destinazione,y_destinazione);

                //pointer_map_sh->taxi_array[index].disponibile = 0; // Setto il taxi come "Non Disponibile"

                pointer_map_sh->taxi_array[index].richiesta.pid_request = pid_richiesta; // Do la richiesta in carico al taxi
                pointer_map_sh->taxi_array[index].richiesta.cella_partenza.x = x_richiesta;
                pointer_map_sh->taxi_array[index].richiesta.cella_partenza.y = y_richiesta;
                pointer_map_sh->taxi_array[index].richiesta.cella_arrivo.x = x_destinazione;
                pointer_map_sh->taxi_array[index].richiesta.cella_arrivo.y = y_destinazione;

                status = 1;

            } else {
                perror("Read Message from Queue : ERROR !");
            }

            idle_time = 0;

        }
        else {

            // IL TAXI DEVE RECARSI ALLA SORGENTE DELLA RICHIESTA :
            if (status == 1 && pointer_map_sh->taxi_array[index].bloccato == 0) {

                if (x_taxi == x_richiesta) { // STESSA RIGA

                    if (y_taxi > y_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi][y_taxi - 1].symbol_in_map == '1') {
                            if (x_taxi != SO_WIDTH - 1) {
                                sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {

                                sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

                                        }

                                    }
                                }
                            }
                        } else {

                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        }
                    } else if (y_taxi < y_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi][y_taxi + 1].symbol_in_map == '1') {
                            if (x_taxi != SO_WIDTH - 1) {
                                sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    destra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        destra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {
                                sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    destra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        destra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            }
                        } else {
                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    }

                } else if (y_taxi == y_richiesta) { // STESSA COLONNA
                    if (x_taxi > x_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {
                            if (y_taxi != SO_HEIGHT - 1) {
                                destra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh != NULL) {
                                    sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh != NULL) {
                                        sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh != NULL) {
                                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else { // y_taxi == SO_HEIGHT - 1
                                sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh != NULL) {
                                    sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh != NULL) {
                                        sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh != NULL) {
                                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

                                        }
                                    }
                                }
                            }
                        } else {
                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    } else if (x_taxi < x_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {
                            if (y_taxi != SO_HEIGHT - 1) {
                                destra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {
                                sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            }
                        } else {
                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    }

                } else if (x_taxi > x_richiesta && y_taxi != y_richiesta) {

                    if (y_taxi < y_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {


                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        } else {


                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

                        }

                    } else if (y_taxi > y_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {

                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        } else {

                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

                        }

                    }
                } else if (x_taxi < x_richiesta && y_taxi != y_richiesta) {

                    if (y_taxi < y_richiesta) { //
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {

                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        } else {

                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        }

                    } else if (y_taxi > y_richiesta) {
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {

                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        } else {

                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;


                        }

                    }

                }
                if (x_taxi == x_richiesta &&
                    y_taxi == y_richiesta) { // STESSA CELLA DELLA RICHIESTA

                    //printf(MAGENTA"\nIo TAXI : %d Sono arrivato alla sorgente della richiesta [%d - %d] !! \n"RESET,pointer_map_sh->taxi_array[index].targa,pointer_map_sh->taxi_array[index].posizione_attuale.x,pointer_map_sh->taxi_array[index].posizione_attuale.y);


                    //status = 0;

                    //pointer_map_sh->taxi_array[index].disponibile = 1;

                    status = 2;

                }

            }
                // IL TAXI DEVE RECARSI ALLA DESTINAZIONE DELLA RICHIESTA :
            else if (status == 2 && pointer_map_sh->taxi_array[index].bloccato == 0) {

                if (x_taxi == x_destinazione) { // STESSA RIGA - COPERTO GIA'
                    if (y_taxi > y_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi][y_taxi - 1].symbol_in_map == '1') {
                            if (x_taxi != SO_WIDTH - 1) {
                                sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {
                                sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            }
                        } else {
                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    } else if (y_taxi < y_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi][y_taxi + 1].symbol_in_map == '1') {
                            if (x_taxi != SO_WIDTH - 1) {
                                sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    destra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        destra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {
                                sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    destra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        destra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } // x_taxi == SO_WIDTH - 1
                        } else {
                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    }
                }
                else if (y_taxi == y_destinazione) { // STESSA COLONNA
                    if (x_taxi > x_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {
                            if (y_taxi != SO_HEIGHT - 1) {
                                destra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else { // y_taxi == SO_HEIGHT - 1
                                sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sopra(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            }
                        } else {
                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    } else if (x_taxi < x_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {
                            if (y_taxi != SO_HEIGHT - 1) {
                                destra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            } else {
                                sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                                time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                    sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                    time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                    if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                        sotto(pointer_map_sh, x_taxi, y_taxi, index);
                                        time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        if (pointer_map_sh->taxi_array[index].bloccato == 0) {
                                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                                        }
                                    }
                                }
                            }
                        } else {

                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    }

                }
                else if (x_taxi > x_destinazione && y_taxi != y_destinazione) {

                    if (y_taxi < y_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {
                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

                        } else {
                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }

                    } else if (y_taxi > y_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi - 1][y_taxi].symbol_in_map == '1') {
                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        } else {
                            sopra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }

                    }
                }
                else if (x_taxi < x_destinazione && y_taxi != y_destinazione) {

                    if (y_taxi < y_destinazione) { //
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {
                            destra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        } else {
                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    } else if (y_taxi > y_destinazione) {
                        if (pointer_map_sh->MAPPA[x_taxi + 1][y_taxi].symbol_in_map == '1') {
                            sinistra(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        } else {
                            sotto(pointer_map_sh, x_taxi, y_taxi, index);
                            time_request += pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;
                        }
                    }

                }

                if (x_taxi == x_destinazione && y_taxi == y_destinazione) {

                    //printf(GREEN"\nTAXI : %d ---> SONO ARRIVATO ALLA DESTINAZIONE [%d - %d]!!!\n "RESET,pointer_map_sh->taxi_array[index].targa,pointer_map_sh->taxi_array[index].posizione_attuale.x,pointer_map_sh->taxi_array[index].posizione_attuale.y);

                    if (time_request >
                        pointer_map_sh->taxi_array[index].max_time_request) { // max_time_request INIZIALMENTE VALE '0'

                        pointer_map_sh->taxi_array[index].max_time_request = time_request;
                    }

                    time_request = 0;

                    idle_time = 0;

                    //pointer_map_sh->requests[get_index_request(pointer_map_sh, pid_richiesta)].type = 0;

                    pointer_map_sh->richieste_successo++;

                    if (pointer_map_sh->taxi_array[index].distance_counter >= pointer_map_sh->taxi_max_cell_counter.distance_counter){

                        pointer_map_sh->taxi_max_cell_counter = pointer_map_sh->taxi_array[index];

                    }

                    if (pointer_map_sh->taxi_array[index].max_time_request >= pointer_map_sh->taxi_max_time.max_time_request){

                        pointer_map_sh->taxi_max_time = pointer_map_sh->taxi_array[index];

                    }

                    if (pointer_map_sh->taxi_array[index].request_counter >= pointer_map_sh->taxi_max_request_counter.request_counter){

                        pointer_map_sh->taxi_max_request_counter = pointer_map_sh->taxi_array[index];
                    }

                    pointer_map_sh->taxi_array[index].disponibile = 1;

                    pointer_map_sh->taxi_array[index].request_counter++;

                    status = 0;

                }

            }

            else if (pointer_map_sh->taxi_array[index].bloccato == 1) {

                gettimeofday(&end_taxi_time, NULL);

                time_taken_taxi = end_taxi_time.tv_sec + end_taxi_time.tv_usec / 1e6 -
                                  start_taxi_time.tv_sec -
                                  start_taxi_time.tv_usec / 1e6; // in seconds

                block_time += time_taken_taxi;

                if (block_time >= SO_TIMEOUT) {

                    pointer_map_sh->taxi_array[index].expired = 1;

                    int x_cella_request_source = pointer_map_sh->requests[get_index_request(
                            pointer_map_sh, pid_richiesta)].cella_partenza.x;
                    int y_cella_request_source = pointer_map_sh->requests[get_index_request(
                            pointer_map_sh, pid_richiesta)].cella_partenza.y;

                    if (pointer_map_sh->MAPPA[x_cella_request_source][y_cella_request_source].num_taxi_presenti >
                        1) {
                        pointer_map_sh->MAPPA[x_cella_request_source][y_cella_request_source].symbol_in_map = 'T';
                    } else {
                        pointer_map_sh->MAPPA[x_cella_request_source][y_cella_request_source].symbol_in_map = '0';
                    }

                    if (pointer_map_sh->MAPPA[x_taxi][y_taxi].num_taxi_presenti > 1) {
                        pointer_map_sh->MAPPA[x_taxi][y_taxi].symbol_in_map = 'T';
                    } else {
                        pointer_map_sh->MAPPA[x_taxi][y_taxi].symbol_in_map = '0';
                    }


                    pointer_map_sh->richieste_abortite++;

                    pointer_map_sh->requests
                    [get_index_request(pointer_map_sh, pid_richiesta)].type = 0;

                    pointer_map_sh->MAPPA[x_taxi][y_taxi].num_taxi_presenti--;

                    shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa

                    releaseSem(sem_taxi);

                    if (semctl(sem_taxi, 0, GETVAL) > 0) {

                        pid_new_taxi = fork();

                        if (pid_new_taxi == 0) {

                            execve(argv_new_taxi[0], argv_new_taxi, envp);

                            exit(EXIT_SUCCESS);

                        }
                        if (pid_new_taxi > 0) {



                            reserveSem(sem_taxi);

                            //releaseSem(request); // COMUNICA AL GESTORE O MAIN DI FAR NASCERE UN NUOVO PROGRAMMA 'request'

                            releaseSem(sem_mem_sh_taxi);

                            reserveSem(sem_final);

                            exit(EXIT_SUCCESS);
                        }

                    }


                }

            }

            else if (pointer_map_sh->taxi_array[index].disponibile == 1){

                gettimeofday(&end_taxi_time, NULL);

                time_taken_taxi = end_taxi_time.tv_sec + end_taxi_time.tv_usec / 1e6 -
                                  start_taxi_time.tv_sec -
                                  start_taxi_time.tv_usec / 1e6; // in seconds

                idle_time += time_taken_taxi;

                if (idle_time >= SO_TIMEOUT) {

                    pointer_map_sh->taxi_array[index].expired = 1;

                    if (pointer_map_sh->MAPPA[x_taxi][y_taxi].num_taxi_presenti > 1) {
                        pointer_map_sh->MAPPA[x_taxi][y_taxi].symbol_in_map = 'T';
                    } else {
                        pointer_map_sh->MAPPA[x_taxi][y_taxi].symbol_in_map = '0';
                    }

                    pointer_map_sh->MAPPA[x_taxi][y_taxi].num_taxi_presenti--;

                    shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa

                    releaseSem(sem_taxi);

                    if (semctl(sem_taxi, 0, GETVAL) > 0) {

                        pid_new_taxi = fork();

                        if (pid_new_taxi == 0) {

                            execve(argv_new_taxi[0], argv_new_taxi, envp);

                            exit(EXIT_SUCCESS);

                        }
                        if (pid_new_taxi > 0) {



                            reserveSem(sem_taxi);

                            releaseSem(sem_mem_sh_taxi);

                            reserveSem(sem_final);

                            exit(EXIT_SUCCESS);
                        }

                    }

                }

            }

        }

        shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa

        releaseSem(sem_mem_sh_taxi);


    } // Fine while COMUNICAZIONE

    signal(SIGINT, SIG_DFL);

    reserveSem(sem_final);


}

