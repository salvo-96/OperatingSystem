//
// Created by Salvatore Cavallaro on 27/05/21.
//
#include "header.h"

typedef struct {
    long mtype; // Per come filtri il messaggio all'interno della coda
    char mtext[DIM_MSG]; // Il messaggio vero e proprio
} msg;

#include "header.h"

int main(void){

    struct timespec tms_generator;

    int pid_request;

    request = semget(SEM_REQUEST_KEY,1,0);

    sem_simulation = semget(SEM_SIMULATION_KEY, 1, 0);

    sem_final = semget(SEM_FINAL_KEY,1,0);

    tms_generator.tv_nsec = 500000000L; // INTERVALLO DI MEZZO SECONDO PER LA GENERAZIOONE DI UNA RICHIESTA
    //tms_generator.tv_nsec = 1000000000L; // INTERVALLO DI UN SEOCNDO PER LA GENERAZIONE DI UNA RICHIESTA
    tms_generator.tv_sec = 0;

    while (semctl(sem_simulation, 0 ,GETVAL) == 1){

        signal(SIGINT, handle_signal_catch_new_request); // IMPOSTA L'HANDLER PER IL SEGNALE ' SIGINT'

        if (semctl(request, 0, GETVAL) > 0) {

            pid_request = fork();

            if(pid_request<0){
                perror("fork request failed : ");
                exit(EXIT_FAILURE);
            }

            if (pid_request == 0) {

                signal(SIGINT, SIG_IGN);

                msg messaggio;

                richiesta_taxi r;

                int pid_taxi_found;

                int index_taxi;

                int index_free_request;

                map *pointer_map_sh;

                queue = msgget(QUEUE_KEY, 0);

                shm = shmget(SHARED_MEMORY_KEY, sizeof(map), 0666);

                request = semget(SEM_REQUEST_KEY, 1, 0);

                sem_mem_sh_request = semget(SEM_SH_REQUEST_KEY, 1, 0);

                while (semctl(sem_mem_sh_request, 0, GETVAL) != 1);

                reserveSem(sem_mem_sh_request);// Riservo il semaforo

                pointer_map_sh = (map *) shmat(shm, NULL, 0);    //Si attacca alla memoria condivisa

                r = genera_nuova_richiesta(pointer_map_sh, getpid());

                if (r.cella_partenza.x >= 0 && r.cella_partenza.y >= 0 && r.cella_arrivo.x >= 0 &&
                    r.cella_arrivo.y >= 0 /*&& (abs(r.cella_arrivo.x - r.cella_partenza.x) +
                                              abs(r.cella_arrivo.y - r.cella_partenza.y) <=
                                              RANGE_PATH)*/) { // Ho trovato una posizione valida sia per l'origine sia per la destinazione della richiesta

                    pid_taxi_found = trova_taxi_vicino(pointer_map_sh, r);

                    index_taxi = get_index_taxi(pointer_map_sh, pid_taxi_found);

                    printf("\nRichiesta : %d - NOTA :: %d\n",getpid(),index_taxi);

                    if (pointer_map_sh->taxi_array[index_taxi].disponibile == 1 &&
                        pointer_map_sh->taxi_array[index_taxi].expired == 0 &&
                        pointer_map_sh->taxi_array[index_taxi].posizione_attuale.x >= 0
                        && pointer_map_sh->taxi_array[index_taxi].posizione_attuale.y >=
                           0) {//SE IL TAXI CHE HO TROVATO E' DISPONIBILE, ALLORA LO CONTATTO (DIVENTA NON DISPONIBILE)

                        
                        pointer_map_sh->MAPPA[r.cella_partenza.x][r.cella_partenza.y].used = 1;

                        index_free_request = get_index_free_request(pointer_map_sh);

                        pointer_map_sh->requests[index_free_request] = r; // Aggiungo la richiesta all'array richieste in posizione i

                        pointer_map_sh->requests[index_free_request].type = 1;

                        if (pointer_map_sh->MAPPA[r.cella_partenza.x][r.cella_partenza.y].symbol_in_map ==
                            'T') {
                            pointer_map_sh->MAPPA[r.cella_partenza.x][r.cella_partenza.y].symbol_in_map = 'S';
                        } else if (
                                pointer_map_sh->MAPPA[r.cella_partenza.x][r.cella_partenza.y].symbol_in_map ==
                                '0') {
                            pointer_map_sh->MAPPA[r.cella_partenza.x][r.cella_partenza.y].symbol_in_map = 'R';
                        }

                        messaggio.mtype = pointer_map_sh->taxi_array[index_taxi].targa;

                        if (snprintf(messaggio.mtext, sizeof(messaggio.mtext), "%d %d %d %d %d",
                                     getpid(),
                                     r.cella_partenza.x, r.cella_partenza.y, r.cella_arrivo.x,
                                     r.cella_arrivo.y) != -1) {
                            if (msgsnd(queue, &messaggio, sizeof(messaggio), 0) == 0) {// Invio la richiesta nella coda di messaggi

                                
                                pointer_map_sh->taxi_array[index_taxi].disponibile = 0;
                                

                                reserveSem(request);


                            } else {
                                perror("msgsnd error");
                            }
                            
                            
                            shmdt((void *) pointer_map_sh);//Si stacca dalla memoria condivisa

                            releaseSem(sem_mem_sh_request);
                            
                            exit(EXIT_SUCCESS);

                            
                        } else {
                            perror("snprintf ERROR !! ");
                        }
                    }else{
                        
                        shmdt((void *) pointer_map_sh);//Si stacca dalla memoria condivisa

                        releaseSem(sem_mem_sh_request);
                        
                        exit(EXIT_SUCCESS);
                    }
                                                  
                    }else{
                        shmdt((void *) pointer_map_sh);//Si stacca dalla memoria condivisa

                        releaseSem(sem_mem_sh_request);
                        
                        exit(EXIT_SUCCESS);
                        
                    }
                
            }
            

            if (pid_request>0){

                wait(NULL);
                
                nanosleep(&tms_generator, NULL);
                
            
                //kill(pid_request, SIGKILL);

                //larm(1);
                //usleep(500000); // Mezzo secondo di intervallo
                //sleep(1); // 1 secondo di intervallo
            }


        }


    }

    


}
