#include "header.h"

int main(void) {

    sem_simulation = semget(SEM_SIMULATION_KEY, 1, 0);

    sem_final = semget(SEM_FINAL_KEY, 1, 0);

    shm = shmget(SHARED_MEMORY_KEY, sizeof(map), 0666);

    int second = 1;

    struct timespec tms_master;

    map *pointer_map_sh;

    tms_master.tv_sec = 1;
    tms_master.tv_nsec = 0;

    while (semctl(sem_simulation, 0, GETVAL) == 1 ) {

        signal(SIGINT,SIG_IGN);

        nanosleep(&tms_master,NULL);

        if (semctl(sem_simulation,0,GETVAL) == 1){

            pointer_map_sh = (map *) shmat(shm, NULL, 0);    //Si attacca alla memoria condivisa

            printf("\n%d-SECONDI\n", second);

            stampa_mappa(pointer_map_sh);

            second++;

            shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa

        }

    }

    signal(SIGINT, SIG_DFL);

}
