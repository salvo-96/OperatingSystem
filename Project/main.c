

#include "header.h"

int main(void) {

    signal(SIGALRM, handle_alarm_simulation);

    pid_t pid_taxi;
    
    char command[50];

    strcpy(command, "pwd | tee path.txt");
    
    system(command);
    
    // Leggo l'output dello script bash nel file 'path.txt'
    char *ch;
    char *file_name = "path.txt";
    
    FILE *fp;
    
    fp = fopen(file_name, "r"); // read mode

    if (fp == NULL){
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    while((ch = fgetc(fp)) != EOF)
        printf("%c", ch);

    fclose(fp);
    
    strcpy(command, "rm path.txt");
    
    system(command);
    
    
    sleep(1);
    
    map *maps; // MAPPA PROVVISORIA PER IsNIZIALIZZAZIONE - VERRA' COPIATA IN *pointer_map_sh

    map *pointer_map_sh; // MAPPA LOCALIZZATA IN SHARED MEMORY - VIENE CARICATA DOPO CHE *maps SIA VALIDA - MAPPA SCRITTA IN SHARED MEMORY
    
    maps = (map *) malloc(sizeof(map));

    // [ 1 ] :
    // CREAZIONE MEMORIA CONDIVISA

    shm = shmget(SHARED_MEMORY_KEY, sizeof(map), IPC_CREAT | IPC_EXCL |
                                                 0666);    //Creazione shared memory per conttenere le info di tutti i processi invitabili
    // [ 2 ] :
    // CREAZIONE SEMAFORI E INIZIALIZZAZIONE
    //scaduto il tempo, il gestore lo imposterà a 0 e gli studenti termineranno le comunicazioni tra loro

    sem_final = semget(SEM_FINAL_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_simulation = semget(SEM_SIMULATION_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_wait_childs = semget(SEM_WAIT_CHILDS_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_mem_sh_request = semget(SEM_SH_REQUEST_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    request = semget(SEM_REQUEST_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_mem_sh_taxi = semget(SEM_SH_TAXI_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_sinc = semget(SEM_SINC_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);    // mutua esclusione inizializzazione dei taxi

    sem_legal = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666);

    sem_taxi = semget(SEM_TAXI_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);

    union semun { //Struttura per modificare valore al semaforo
        int val; //Questa variabile deve contenere il valore con il quale il semaforo verrà inizializzato
        struct semid_ds *buf;
        unsigned short *array;
#if defined(__linux__)
        struct seminfo* __buf;
#endif
    };

    union semun sem_arg;//Variabile struct semaforo per inizializzare i vari semafori

    sem_arg.val = 1;

    semctl(sem_sinc, 0, SETVAL,
           sem_arg);//Inizializzazione del semaforo Che diventa 0 una volta scaduto l'ALARM del processo master

    semctl(sem_mem_sh_taxi, 0, SETVAL, sem_arg);

    sem_arg.val = 0;

    semctl(sem_taxi, 0, SETVAL, sem_arg);

    semctl(new_request, 0, SETVAL, sem_arg);

    sem_arg.val = 1;

    semctl(sem_simulation, 0, SETVAL,
           sem_arg);     //Inizializzazione del semaforo,  Che diventa 0 una volta che TUTTI i processi Taxi hanno terminato alla fine della simulazione

    semctl(sem_mem_sh_request, 0, SETVAL, sem_arg);

    semctl(sem_legal, 0, SETVAL,
           sem_arg);//Inizializzazione del semaforo, verifica le poszioni legali di SO_HOLES

    //  semctl(sem_mem_sh_taxi, 0, SETVAL, sem_arg);

    sem_arg.val = SO_TAXI;

    semctl(sem_wait_childs, 0, SETVAL,
           sem_arg);      //Inizializzazione del semaforo,  Che diventa 0 una volta che TUTTI i processi Taxi hanno finito di inizializzarsi

    semctl(sem_final, 0, SETVAL, sem_arg);

    sem_arg.val = SO_SOURCE;

    semctl(request, 0, SETVAL, sem_arg);

    // CREAZIONE CODA DI MESSAGGI E DICHIARAZIONE STRUTTURA MESSAGGIO DA CONDIVIDERE:

    queue = msgget(QUEUE_KEY, IPC_CREAT | IPC_EXCL | 0666); //coda invio richieste

    //struct msqid_ds struttura;//Struttura di controllo sullo stato della coda dei messaggi

    // ------------------------------- INIZIALIZZAZIONE MAPPA - SCRITTURA IN MEMORIA CONDIVISA ----------------------------------------------

    while (semctl(sem_legal, 0, GETVAL) != 0) {
        posizione_casuale(maps); // ****
    }

    // ----- ACCEDO ALLA MEMORIA CONDIVISA ----
    pointer_map_sh = (map *) shmat(shm, NULL, 0);    //Si attacca alla memoria condivisa

    pointer_map_sh->j = 0; // Inizializzo la variabile indice per l'array taxi

    pointer_map_sh->richieste_abortite = 0;

    pointer_map_sh->richieste_successo = 0;

    // STO SCRIVENDO IN MEMORIA CONDIVISA LA MAPPA DELLA CITTA' :
    for (int k = 0; k < SO_WIDTH; k++) {
        for (int l = 0; l < SO_HEIGHT; l++) {
            pointer_map_sh->MAPPA[k][l] = maps->MAPPA[k][l];
        }
    }

    for (int i = 0; i < SO_SOURCE; ++i) {
        pointer_map_sh->requests[i].type = 0; // Rendo la richiesta VUOTA - DA SOSTITUIRE
    }

    shmdt((void *) pointer_map_sh);    //Si stacca dalla memoria condivisa
    // ----- ESCO DALLA MEMORIA CONDIVISA ----

    free(maps);


    for (int i = 0; i < SO_TAXI; ++i) {

        pid_taxi = fork();

        if (pid_taxi == 0) { // Figlio

            execve(argv_taxi[0], argv_taxi, envp);

            exit(EXIT_SUCCESS);
        }


    }

    if (pid_taxi > 0) {

        while (semctl(sem_wait_childs, 0, GETVAL) != 0);//Attende che tutti i processi taxi si siano inizializzati

        struct timeval start, end;

        int pid_request_generator;

        int pid_handler_print;

        printf("\n --- --- --- --- --- --- --- --- INIZIO DELLA SIMULAZIONE --- --- --- --- --- --- --- ---\n");

        gettimeofday(&start, NULL);

        pid_handler_print = fork();

        if (pid_handler_print == 0) {
            execve(argv_master[0], argv_master, envp);
            exit(EXIT_SUCCESS);
        }

        if (pid_handler_print > 0) {

            pid_request_generator = fork();

            if (pid_request_generator == 0) {

                execve(argv_request_generator[0], argv_request_generator,envp);
                exit(EXIT_SUCCESS);
                
            }

            if (pid_request_generator > 0) {

                signal(SIGINT, SIG_IGN);

                alarm(SO_DURATION);

                while(semctl(sem_simulation,0,GETVAL) == 1);

                gettimeofday(&end, NULL);

                double time_taken = end.tv_sec + end.tv_usec / 1e6 -
                                    start.tv_sec - start.tv_usec / 1e6; // in seconds

                printf("\nLA SIMULAZIONE HA IMPIEGATO %f-SECONDI\n", time_taken);

                printf("\nASPETTO CHE TUTTI I PROCESSI TAXI FINISCANO ...\n");

                while (semctl(sem_final, 0, GETVAL) !=0);// SEMAFORO CHE SEGNA LA FINE DELLA SIMULAZIONE, SCATTA DALLA FINE DELL'ALLARM DELL'INIZIO DELLA SIMULAZIONE

                printf("\nI PROCESSI TAXI HANNO FINITO.\n");

                usleep(500000);

                signal(SIGINT, SIG_DFL);

                printf("\n\n\n--- --- --- --- --- MAPPA FINALE --- --- --- --- ---\n");

                printf(BOLDYELLOW"\nLEGENDA MAPPA : "RESET);

                printf("\n\t- Celle Inaccessibili (SO_HOLES) '1' : ");
                printf(BOLDRED"Rosso\n"RESET);
                printf("\n\t- Sorgenti richieste generate (SO_SOURCE) 'R' : ");
                printf(BOLDCYAN"Ciano\n"RESET);
                printf("\n\t- Celle piu' attraversate (SO_TOP_CELLS) '#' : ");
                printf(BOLDGREEN"Verde\n\n"RESET);

                pointer_map_sh = (map *) shmat(shm, NULL, 0);//Si attacca alla memoria condivisa

                stampa_mappa_finale(pointer_map_sh);

                msgctl(queue, IPC_STAT, &struttura);

                stampa_numero_taxi_in_mappa(pointer_map_sh);
                stampa_numero_richieste_in_mappa(pointer_map_sh, struttura.msg_qnum);
                stampa_numero_celle_inaccessibili(pointer_map_sh);

                printf("\n");

                printf("\nValore semaforo request : %d\n", semctl(request,0,GETVAL));

                printf("\nIl taxi che ha soddisfatto più richieste durante tutta la simulazione e' : %d -- "
                       "Richieste completate : %d\n", pointer_map_sh->taxi_max_request_counter.targa,
                       pointer_map_sh->taxi_max_request_counter.request_counter);
                printf("\nIl taxi che ha fatto il viaggio piu' lungo durante tutta la simulazione e' : %d -- Celle attraversate : %d\n",
                       pointer_map_sh->taxi_max_cell_counter.targa,
                       pointer_map_sh->taxi_max_request_counter.distance_counter);
                printf("\nIl taxi che ha impiegato più tempo per soddisfare una richiesta durante tutta la simulazione e' : %d -- Tempo più lungo : %ld\n",
                       pointer_map_sh->taxi_max_time.targa, pointer_map_sh->taxi_max_time.max_time_request);

                shmdt((void *) pointer_map_sh);//Si stacca dalla memoria condivisa

                msgctl(queue, IPC_STAT, &struttura);
                printf("\nMessaggi sulla coda a tempo scaduto: %ld \n", struttura.msg_qnum);
                printf("Numero massimo byte della coda: %ld \n", struttura.msg_qbytes);
                printf("Numero bytes sulla coda a tempo scaduto: %ld \n\n", struttura.msg_cbytes);

                //  ***   FINE   ***

                printf("\n ---  FINE SIMULAZIONE  --- \n");

                printf("Sto deallocando le risorse IPC utilizzate \n");

                msgctl(queue, IPC_RMID, NULL);
                shmctl(shm, IPC_RMID, NULL);

                semctl(sem_final, 0, IPC_RMID);
                semctl(sem_wait_childs, 0, IPC_RMID);
                semctl(sem_simulation, 0, IPC_RMID);
                semctl(sem_sinc, 0, IPC_RMID);
                semctl(sem_legal, 0, IPC_RMID);
                semctl(request, 0, IPC_RMID);
                semctl(sem_taxi, 0, IPC_RMID);
                semctl(sem_mem_sh_taxi, 0, IPC_RMID);
                semctl(sem_mem_sh_request, 0, IPC_RMID);

                return 0;
            }
        }
    }
}
