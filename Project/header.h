
#ifndef PROGETTO_SO_CON_SIGNAL_HEADER_H
#define PROGETTO_SO_CON_SIGNAL_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sem.h>
#include <libc.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define SO_WIDTH 20
#define SO_HEIGHT 10
#define SO_HOLES 10
#define SO_SOURCE 10 // Numero di punti sulla matrice che rappresentano le richieste (aereoporti, stazioni)
#define SO_TAXI 10
#define SO_CAP_MIN 1
#define SO_CAP_MAX 1
#define SO_TIMENSEC_MIN 10000000
#define SO_TIMENSEC_MAX 30000000
#define SO_DURATION 20
#define SO_TIMEOUT 1
#define SO_TOP_CELLS 40

#define DIM_MSG 256

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#define SHARED_MEMORY_KEY 1234

#define SEM_WAIT_CHILDS_KEY 4321
#define SEM_SIMULATION_KEY 3214
#define SEM_FINAL_KEY 1100
#define QUEUE_KEY 5500
#define SEM_SH_TAXI_KEY 1991
#define SEM_SH_REQUEST_KEY 2587
#define SEM_SINC_KEY 1220
#define SEM_REQUEST_KEY 6363
#define SEM_TAXI_KEY 8419




/*
 VALUE OF SYMBOLS IN MAP :
 0 -> CELLA DI TIPO ACCESSIBILE
 1 -> CELLA DI TIPO INACCESSIBILE
 S -> CELLA OCCUPATA SIA DA UN TAXI SIA DA UNA RICHIESTA
 T -> CELLA OCCUPATA DA UN TAXI
 R -> CELLA OCCUPATA DA UNA RICHIESTA
 */



char * const argv_new_taxi[] = {"/Users/salvo/Desktop/IMPORTANTE/progetto_SO/new_taxi", NULL};
char * const argv_master[] = {"/Users/salvo/Desktop/IMPORTANTE/progetto_SO/master", NULL};
char * const argv_taxi[] = {"/Users/salvo/Desktop/IMPORTANTE/progetto_SO/taxi", NULL};
char * const argv_request[] = {"/Users/salvo/Desktop/IMPORTANTE/progetto_SO/request", NULL};
char * const argv_request_generator[] = {"/Users/salvo/Desktop/IMPORTANTE/progetto_SO/request_generator", NULL};
char * const envp[] = {NULL};

// FLAG per VALIDARE LA MAPPA CON POSIZIONI LEGALI DELLE CELLE SO_HOLES (inaccessibili)
int valid;
//CONTATORE RICHIESTE SO_SOURCES SCATTATE ALL'ACQUISIZIONE DEL SEGNALE SIGINT DA stdin
int counter_request_signal;
// Memoria Condivisa :
int shm;
// Coda Messaggi :
int queue;

// Semafori :
int sem_sinc; //Semaforo sem_sinc dichiarato globalmente perchè così potrà essere decrementato dall'handler del segnale alarm
int sem_legal;
int sem_final;
int sem_wait_childs;
int sem_simulation;
int sem_mem_sh_request;
int sem_mem_sh_taxi;
int request;
int sem_taxi;

int new_request;

struct msqid_ds struttura;//Struttura di controllo sullo stato della coda dei messaggi

struct timeval t1;

int second_simulation = 1;

//Funzioni per prendere/rilasciare il semaforo binario
void reserveSem(int semId) {
    struct sembuf sops[1];
    sops[0].sem_num = 0;
    sops[0].sem_op = -1; //Decrementa Valore
    sops[0].sem_flg = 0;
    semop(semId, sops, 1);
}
void releaseSem(int semId) {
    struct sembuf sops[1];
    sops[0].sem_num = 0;
    sops[0].sem_op = 1; //Incrementa Valore
    sops[0].sem_flg = 0;
    semop(semId, sops, 1);
}


typedef struct {
    int x;
    int y;
}coordinate;

struct timespec tms;

typedef struct { //Struttura del Taxi -- PROCESSO TAXI --

    int type;

    pid_t pid_request; // Mi permette di sapere quale taxi si sta occupando della mia richiesta.

    coordinate cella_partenza; // Indica da dove si origina la richiesta, dove si trova il cliente.

    coordinate cella_arrivo; // Indica la destinazione da raggiungere.

}richiesta_taxi;

typedef struct taxi_t{
    pid_t targa;
    int expired;
    int bloccato; // 0 -> Non bloccato , 1 ->Bloccato
    richiesta_taxi richiesta;
    coordinate posizione_attuale;
    time_t max_time_request; // Si aggiorna a ogni nuova richiesta completata, indica il viaggio più lungo effettuato dal taxi per tutta la simulazione.
    // Se la richiesta successiva dovesse essere più breve, ovviamente non si aggiornerà tale valore.
    int disponibile; // 1 = Disponibile , 0 = Non Disponibile, è già occupato in un altra corsa
    int distance_counter; // Indica il cammino totale per tutta la durata della simulazione, viene incrementata ad ogni cella attraversata
    int request_counter; // Indica quante richieste il taxi ha soddisfatto, durante tutta la durata della simulazione
}taxi;

typedef struct { //Struttura della cella
    int max_taxi_in_cella; // Indica il massimo numero di taxi che può ospitare contemporaneamente la cella ( estratto casualmente tra SO_CAP_MINe SO_CAP_MAX )
    int used; // 0-> Indica che non è stata usata in precedenza come richiesta di SO_SOURCES --- 1 -> E' gia stata usata! Non utilizzabile !
    char symbol_in_map;  // 0 -> Accessibile , 1 -> Inacessibile (Barriera)
    coordinate coordinates; // Coordinate della cella presente in mappa
    time_t tempo_attraversamento; // Tempo di attraversamento per cella ( estratto casualmente fra SO_TIMENSEC_MIN e SO_TIMENSEC_MAX )
    //taxi *taxi; //  Taxi presente in cella
    int num_taxi_presenti; // 1 -> 1 taxi presente nella cella , 0 -> nessun taxi presente (cella vuota).
    int crossing_counter; // Misura quante volte è stata attraversata una determinata cella, serve per la STAMPA
}cella;

typedef struct {
    taxi taxi_max_request_counter;
    taxi taxi_max_time;
    taxi taxi_max_cell_counter;
    cella MAPPA[SO_WIDTH][SO_HEIGHT];
    taxi taxi_array[SO_TAXI];
    int j;
    richiesta_taxi requests[SO_SOURCE*2];
    int richieste_abortite;
    int richieste_successo;
}map;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void stampa_mappa(map *pointer_map_sh){

    for (int i = 0; i < SO_TAXI; ++i) {

        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[i].posizione_attuale.x][pointer_map_sh->taxi_array[i].posizione_attuale.y].symbol_in_map = 'T';

    }

    for (int k = 0; k < SO_WIDTH; k++) {
        for (int l = 0; l < SO_HEIGHT; l++) {
            if (pointer_map_sh->MAPPA[k][l].symbol_in_map == 'T' ||
                pointer_map_sh->MAPPA[k][l].symbol_in_map == 'S') {
                fprintf(stdout, BOLDBLUE"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
                //printf(BOLDBLUE"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
            } else if (pointer_map_sh->MAPPA[k][l].symbol_in_map == '1') {
                fprintf(stdout, BOLDRED"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
                //printf(BOLDRED"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
            } else if (pointer_map_sh->MAPPA[k][l].symbol_in_map == 'R') {
                fprintf(stdout, BOLDMAGENTA"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
                //printf(BOLDMAGENTA"| %c |"RESET, pointer_map_sh->MAPPA[k][l].symbol_in_map);
            } else {
                fprintf(stdout, "| %c |", pointer_map_sh->MAPPA[k][l].symbol_in_map);
                //printf("| %c |", pointer_map_sh->MAPPA[k][l].symbol_in_map);
            }
        }
        fprintf(stdout,"\n");

        //printf("\n");
    }

    fflush(stdout);

    second_simulation++;

}


int random_coordinate(int range){

    srand(clock());

    int c;
    c = rand() % (range);
    //printf("\nval c : %d\n",c);
    return c;
}

coordinate estrai_casualmente_taxi_cell(map *mappa){

    coordinate taxi;

    int x_taxi = random_coordinate(SO_WIDTH);

    int y_taxi = random_coordinate(SO_HEIGHT);

    if(x_taxi<=SO_WIDTH && y_taxi<=SO_HEIGHT && x_taxi>=0 && y_taxi>= 0 && (mappa->MAPPA[x_taxi][y_taxi].symbol_in_map == '0' && mappa->MAPPA[x_taxi][y_taxi].num_taxi_presenti < mappa->MAPPA[x_taxi][y_taxi].max_taxi_in_cella )||
       ((mappa->MAPPA[x_taxi][y_taxi].symbol_in_map == 'T' || mappa->MAPPA[x_taxi][y_taxi].symbol_in_map == 'S')
        && mappa->MAPPA[x_taxi][y_taxi].num_taxi_presenti < mappa->MAPPA[x_taxi][y_taxi].max_taxi_in_cella)) { // Se è di tipo accessibile

        //printf("\nValore max_taxi_in_cella : %d , Valore num_taxi_presenti : %d . Per la cella in posizione [%d-%d]\n",mappa->MAPPA[x_taxi][y_taxi].max_taxi_in_cella,mappa->MAPPA[x_taxi][y_taxi].num_taxi_presenti,x_taxi,y_taxi);

        taxi.x = x_taxi; // COINCIDE CON QUELLE DI PARTENZA, OVVERO DA DOVE SI E' GENERATA LA POSZIONE DEL TAXI
        taxi.y = y_taxi; // COINCIDE CON QUELLE DI PARTENZA


    } else{
        taxi.x = -1;
        taxi.y = -1;
    }

    return taxi;

}

richiesta_taxi genera_nuova_richiesta(map *mappa,pid_t pid) {

    richiesta_taxi richiesta;

    richiesta.pid_request = pid;

    int x_request_start = random_coordinate(SO_WIDTH);

    int y_request_start = random_coordinate(SO_HEIGHT);

    //printf("\nREQUEST : x = %d  Y = %d\n",x_request_start,y_request_start);

    if ((mappa->MAPPA[x_request_start][y_request_start].symbol_in_map == '0' &&
         mappa->MAPPA[x_request_start][y_request_start].used == 0) || (mappa->MAPPA[x_request_start][y_request_start].symbol_in_map == 'T' &&
                                                                       mappa->MAPPA[x_request_start][y_request_start].used == 0)){ // LE RICHIESTE SI POSSONO GENERARE SOLO NELLE CELLE LIBERE
        // NON USATE PRIMA PER SO_SOURCES OPPURE IN CELLE OVE VI SONO TAXI E NON USATE PRECEDENTEMENTE PER SO_SOURCES
        richiesta.cella_partenza.x = x_request_start;
        richiesta.cella_partenza.y = y_request_start;

    }else{ // SE LE COORDINATE NON RISPETTANO IL VINCOLO DI CUI SOPRA, ALLORA RITORNO VALORI DI ERRORE
        richiesta.cella_partenza.x = -1;
        richiesta.cella_partenza.y = -1;
    }

    // CALCOLO LA DESTINAZIONE :

    int x_request_dest = random_coordinate(SO_WIDTH);
    int y_request_dest = random_coordinate(SO_HEIGHT);

    if ((mappa->MAPPA[x_request_dest][y_request_dest].symbol_in_map=='0' || mappa->MAPPA[x_request_dest][y_request_dest].symbol_in_map == 'T' ||
         mappa->MAPPA[x_request_dest][y_request_dest].symbol_in_map == 'S' || mappa->MAPPA[x_request_dest][y_request_dest].symbol_in_map == 'R') &&
        x_request_dest!= richiesta.cella_partenza.x && y_request_dest != richiesta.cella_partenza.y){
        richiesta.cella_arrivo.x = x_request_dest;
        richiesta.cella_arrivo.y = y_request_dest;
    }else{ // SE LE COORDINATE NON RISPETTANO IL VINCOLO DI CUI SOPRA, ALLORA RITORNO VALORI DI ERRORE
        richiesta.cella_arrivo.x = -1;
        richiesta.cella_arrivo.y = -1;
    }

    return richiesta;
}

time_t estrai_tempo_random(int lower, int upper){
    srand(clock());
    int value = (rand() % (upper-lower +1)) + lower;
    return value;
}

int estrai_max_number_taxi_cell_random(int lower, int upper){
    srand(clock());
    int value = (rand() % (upper-lower +1)) + lower;

    return value;
}

void swap(int* xp, int* yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void selectionSort(int arr[], int n)
{
    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < n - 1; i++) {

        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i + 1; j < n; j++)
            if (arr[j] < arr[min_idx])
                min_idx = j;

        // Swap the found minimum element
        // with the first element
        swap(&arr[min_idx], &arr[i]);
    }
}

void swapTopCells(int matrix[SO_WIDTH*SO_HEIGHT][3], int row1, int row2){

    for (int i = 0; i < 3; ++i)
    {
        int t = matrix[row1][i];
        matrix[row1][i] = matrix[row2][i];
        matrix[row2][i] = t;
    }
}

void selectionSortTopCells(int matrix[SO_WIDTH*SO_HEIGHT][3]){

    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < SO_WIDTH*SO_HEIGHT - 1; i++) {

        // Find the minimum element in unsorted array
        min_idx = i;

        for (j = i + 1; j < SO_WIDTH*SO_HEIGHT; j++)
            if (matrix[j][0] < matrix[min_idx][0])
                min_idx = j;

        // Swap the found minimum element
        // with the first element
        swapTopCells(matrix,min_idx,i);
    }

}

int check_same_values_in_array(int array[SO_HOLES]){

    int duplicates = 1; // 1 := false
    // 0 := true

    for (int i = 0; i < SO_HOLES - 1; i++) {
        for (int j = i + 1; j < SO_HOLES; j++) {
            if (array[i] == array[j]) {
                duplicates = 0;
            }
        }
    }
    return duplicates;
} // Controlla se le posizioni casuali generate per SO_HOLES non sono ripetute

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void stampa_matrice_con_holes(map *matrix_map, int holes_vet[SO_HOLES]) {

    int i = 0; // riga
    int j = 0; // colonna
    coordinate holes[SO_HOLES];
    int position_hole = 0;
    int position_in_matrix = 0;
    int position_in_vet = 0;
    while (i < SO_HEIGHT) {
        if(position_in_matrix!=holes_vet[position_in_vet]) {
            matrix_map->MAPPA[j][i].symbol_in_map = '0';
        }else if(i == holes_vet[position_in_vet]){
            coordinate actual_hole;
            actual_hole.x = j;
            actual_hole.y = i;
            holes[position_hole] = actual_hole;
            position_hole++;
            position_in_vet++;
            matrix_map->MAPPA[j][i].symbol_in_map = '1';
            // NON OCCORE METTER IL TEMPO DI ATTRAVERAMENTO E IL # MAX DI TAXI , IN QUANTO LA CELLA E' DI TIPO INACCESSIBILE
        }
        i++;
        position_in_matrix++;
    }
    i = 0;
    while (j < SO_WIDTH) {
        printf("\n");
        j++;
        while (i < SO_HEIGHT) {
            if(position_in_matrix!=holes_vet[position_in_vet]){
                matrix_map->MAPPA[j][i].symbol_in_map = '0';
            }else if(position_in_matrix == holes_vet[position_in_vet]){
                coordinate actual_hole;
                actual_hole.x = j;
                actual_hole.y = i;
                holes[position_hole] = actual_hole;
                position_hole++;
                position_in_vet++;
                matrix_map->MAPPA[j][i].symbol_in_map = '1';
            }
            i++;
            position_in_matrix++;
        }
        i = 0;
    }

    i = 0;
    valid = 0;
    for (; i<SO_HOLES - 1; i++){

        // valid == 0 --> sto trovando posizioni LEGALI e posso
        // continuare il ciclo per capire se gli altri hole hanno posizioni valide, se cosi non fosse, valid sarà settato == 1 e si uscirà dal ciclo.
        // In quanto è necessario che solo un caso non sia valido affinchè tutte le posizioni degli SO_HOLES sia ILLEGALI !!

        coordinate* actual_hole = ( coordinate*) malloc(sizeof(coordinate));

        actual_hole->x = holes[i].x;

        actual_hole->y = holes[i].y;

        //printf("TEST %d-nodo ------>  x = %d - y = %d\n" ,i , actual_hole->x, actual_hole->y);

        j = i;

        for(;j<SO_HOLES;j++){ // Controlla se sotto il valore puntato, per tutti gli elementi ne esiste uno sotto

            if(actual_hole->x + 1 == holes[j+1].x && actual_hole->y == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova SOTTO l'elemento in posizione %d\n",j+1,i);
                valid = 1;
            } // (i+1) sta sotto (i) --> ILLEGALE !
        }

        j = 0;

        for(;j<SO_HOLES;j++) {
            if (actual_hole->x - 1 == holes[j + 1].x && actual_hole->y == holes[j + 1].y){
                //printf("\nL'elemento in posizione %d si trova SOPRA dell'elemento in posizione %d\n", j + 1, i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x == holes[j+1].x && actual_hole->y + 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova A DESTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x == holes[j+1].x && actual_hole->y - 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova A SINISTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x + 1 == holes[j+1].x && actual_hole->y + 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova SOTTO A DESTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x + 1 == holes[j+1].x && actual_hole->y - 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova SOTTO A SINISTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x - 1 == holes[j+1].x && actual_hole->y - 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova SOPRA A SINISTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }

        j = 0;

        for(;j<SO_HOLES;j++){
            if(actual_hole->x - 1 == holes[j+1].x && actual_hole->y + 1 == holes[j+1].y){
                //printf("\nL'elemento in posizione %d si trova SOPRA A DESTRA dell'elemento in posizione %d\n",j+1,i);
                valid = 1;
            }
        }


        free(actual_hole);

    }

    if(valid==0){
        for (int k = 0; k < SO_WIDTH; k++) {
            for (int l = 0; l < SO_HEIGHT; l++) {
                if (matrix_map->MAPPA[k][l].symbol_in_map == '0'){
                    matrix_map->MAPPA[k][l].coordinates.x = k;
                    matrix_map->MAPPA[k][l].coordinates.y = l;
                    matrix_map->MAPPA[k][l].tempo_attraversamento = estrai_tempo_random(SO_TIMENSEC_MIN,SO_TIMENSEC_MAX);
                    matrix_map->MAPPA[k][l].max_taxi_in_cella = estrai_max_number_taxi_cell_random(SO_CAP_MIN,SO_CAP_MAX);
                    matrix_map->MAPPA[k][l].num_taxi_presenti = 0;
                    matrix_map->MAPPA[k][l].crossing_counter = 0;
                    matrix_map->MAPPA[k][l].used = 0;
                } else {
                    matrix_map->MAPPA[k][l].coordinates.x = k;
                    matrix_map->MAPPA[k][l].coordinates.y = l;
                }
            }
        }
        reserveSem(sem_legal);
    }
}

void posizione_casuale(map *matrix){

    //map *map_legal;

    srand(clock());

    int j;

    int has_duplicates;

    int indexs_val[SO_HOLES];

    int val_random;
    //int val_random_y;

    printf("\n");

    for(int i = 0; i<SO_HOLES; i++) {
        val_random = rand() % ((SO_WIDTH * SO_HEIGHT)+1); // rand() % (max_number + 1 - minimum_number) + minimum_number
        //val_random_y = rand() % (SO_HEIGHT +1); // rand() % (max_number + 1 - minimum_number) + minimum_number
        //printf("%d ,", val_random);
        indexs_val[i] = val_random;
    }

    has_duplicates = check_same_values_in_array(indexs_val);

    //printf("\nHa duplicati : ");

    if(has_duplicates==1) {

        selectionSort(indexs_val,sizeof(indexs_val)/sizeof (int)); // Ordino i valori in modo crescente

        stampa_matrice_con_holes(matrix,indexs_val);

    }

}

void stampa_numero_richieste_in_mappa(map *pointer_map_sh,int richieste_inevase) {

    printf(BOLDRED"\nRICHIESTE COMPLETATE CON SUCCESSO : %d\n"RESET,pointer_map_sh->richieste_successo);
    printf(BOLDRED"\nRICHIESTE INEVASE  : %d\n"RESET,richieste_inevase);
    printf(BOLDRED"\nRICHIESTE ABORTITE  : %d\n"RESET,pointer_map_sh->richieste_abortite);
}

void stampa_numero_celle_inaccessibili(map *mappa) {
    int counter_so_holes = 0;
    for (int k = 0; k < SO_WIDTH; k++) {
        for (int l = 0; l < SO_HEIGHT; l++) {
            if (mappa->MAPPA[k][l].symbol_in_map == '1') {
                counter_so_holes++;
            }
        }
    }
    printf("\nNELLA MAPPA SONO PRESENTI -> %d CELLE INACCESSIBILI\n", counter_so_holes);
}

void stampa_numero_taxi_in_mappa(map *mappa) {
    int counter_taxi_in_cella = 0;
    for (int k = 0; k < SO_TAXI; k++) {
        if (mappa->taxi_array[k].targa!=0)
            counter_taxi_in_cella++;
    }
    printf("\nNELLA MAPPA SONO PRESENTI -> %d TAXI\n", counter_taxi_in_cella);
}

void stampa_top_cells(map* mappa){

    int temp_cells[SO_HEIGHT*SO_WIDTH][3];
    //int (*temp_cells)[SO_WIDTH*SO_HEIGHT] = malloc(sizeof (int[SO_WIDTH*SO_HEIGHT][3]));

    int index = 0;

    for (int i = 0; i < SO_WIDTH; ++i) {
        for (int j = 0; j < SO_HEIGHT; ++j) {
            temp_cells[index][0] = mappa->MAPPA[i][j].crossing_counter;
            temp_cells[index][1] = mappa->MAPPA[i][j].coordinates.x;
            temp_cells[index][2] = mappa->MAPPA[i][j].coordinates.y;
            index++;
        }
    }

    selectionSortTopCells(temp_cells);

    int p = 1;
    for (int i = SO_WIDTH*SO_HEIGHT-1; i >= SO_WIDTH*SO_HEIGHT-SO_TOP_CELLS; --i) {
        mappa->MAPPA[temp_cells[i][1]][temp_cells[i][2]].symbol_in_map = '#';
        p++;
    }


    for (int k = 0; k < SO_WIDTH; k++) {
        for (int l = 0; l < SO_HEIGHT; l++) {
            if (mappa->MAPPA[k][l].used == 1){
                printf(BOLDMAGENTA"| R |"RESET);
            }else if(mappa->MAPPA[k][l].symbol_in_map == '#' ){
                printf(BOLDGREEN"| %c |"RESET, mappa->MAPPA[k][l].symbol_in_map);
            } else if(mappa->MAPPA[k][l].symbol_in_map == 'T' || mappa->MAPPA[k][l].symbol_in_map == '0'){
                printf("| 0 |");
            }else if(mappa->MAPPA[k][l].symbol_in_map == '1'){
                printf(BOLDRED"| 1 |"RESET);
            }


        }
        printf("\n");
    }



}

void stampa_mappa_finale(map* pointer_map_sh){

    stampa_top_cells(pointer_map_sh);


}

void handle_sigint(int sig){

    printf("Caught signal -> %d  !!!!\n", sig);

    //Rimuove la shared memory :
    shmctl(shm, IPC_RMID, NULL);

    //Rimuove i semafori :
    semctl(sem_legal, 0, IPC_RMID);
    semctl(sem_simulation, 0, IPC_RMID);
    semctl(sem_final, 0, IPC_RMID);
    semctl(sem_wait_childs, 0, IPC_RMID);
    semctl(sem_sinc, 0, IPC_RMID);
    semctl(request,0,IPC_RMID);
    semctl(sem_mem_sh_request, 0 ,IPC_RMID);
    semctl(sem_mem_sh_taxi, 0, IPC_RMID);


    //Rimuove la code di messaggi :
    msgctl(queue, IPC_RMID, NULL);

}

void handle_alarm_simulation(){
    reserveSem(sem_simulation);
}

void handle_signal_catch_new_request(){

    releaseSem(request);

    printf("\nsemctl(request) : %d\n", semctl(request,0,GETVAL));

    signal(SIGINT, handle_signal_catch_new_request); // IMPOSTA L'HANDLER PER IL SEGNALE ' SIGINT'
    //reserveSem(sem_sinc); // Lo faccio valere 0 quando ci sono richieste . Se vale 1 (come inizializzazione) -> vorrà dire che non ci sono richieste

}


int get_index_expired_taxi(map* mappa){

    int i = 0;

    int index;

    while (i<SO_TAXI){
        //printf("\ntaxi : %d - i : %d\n",mappa->taxi_array[i].targa,i);
        if (mappa->taxi_array[i].expired == 1){
            index = i;
        }
        i++;
    }

    return index;

}

int get_index_taxi(map* mappa, int pid){

    int i = 0;

    int index;

    while (i<SO_TAXI){
        //printf("\ntaxi : %d - i : %d\n",mappa->taxi_array[i].targa,i);
        if (mappa->taxi_array[i].targa == pid){
            index = i;
        }
        i++;
    }

    return index;

}

int get_index_request(map* mappa, int pid_richiesta){

    int index;

    for (int i = 0; i < SO_SOURCE*2; ++i) {
        if (mappa->requests[i].pid_request == pid_richiesta){
            index = i;
        }
    }

    return index;
}

int get_index_free_request(map* mappa){

    int index;

    for (int i = 0; i < SO_SOURCE*2; ++i) {
        if (mappa->requests[i].type == 0){
            index = i;
        }
    }
    return index;
}

int trova_taxi_vicino(map *mappa,richiesta_taxi request) { //

    int final_cost;
    int path;
    int x;
    int y;

    int pid;

    coordinate c;

    c.x =-1;
    c.y = -1;

    for (int i = 0; i < SO_TAXI; i++) {

        if (mappa->taxi_array[i].disponibile==1 && mappa->taxi_array[i].expired == 0){

            x = mappa->taxi_array[i].posizione_attuale.x - request.cella_partenza.x;

            y = mappa->taxi_array[i].posizione_attuale.y - request.cella_partenza.y;

            path = abs(x) + abs(y);

            if (path<final_cost){

                final_cost = path;

                c.x = mappa->taxi_array[i].posizione_attuale.x;

                c.y = mappa->taxi_array[i].posizione_attuale.y;

                pid = mappa->taxi_array[i].targa;
            }

        }

    }

    return pid; // Mi ritorna il taxi in posizione più conveniente per la richiesta data come parametro alla funzione
}

void sotto(map* pointer_map_sh, int x_taxi, int y_taxi, int index) {

    if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x+1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti <
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x+1][pointer_map_sh->taxi_array[index].posizione_attuale.y].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);
        pointer_map_sh->taxi_array[index].bloccato = 0;

        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].crossing_counter++;
        pointer_map_sh->taxi_array[index].max_time_request += pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].tempo_attraversamento;
        pointer_map_sh->taxi_array[index].distance_counter++;

        // pointer_map_sh->MAPPA[x_taxi+1][y_taxi].taxi[pointer_map_sh->MAPPA[x_taxi+1][y_taxi].num_taxi_presenti] = pointer_map_sh->MAPPA[x_taxi][y_taxi].taxi;
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti--;
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x + 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti++;

        if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti > 1) {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x + 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';

        } else {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = '0';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x + 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
        }

        pointer_map_sh->taxi_array[index].posizione_attuale.x++;


    } else if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x+1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti ==
               pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x+1][pointer_map_sh->taxi_array[index].posizione_attuale.y].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);

        pointer_map_sh->taxi_array[index].bloccato = 1;



    }

}

void sinistra(map* pointer_map_sh, int x_taxi, int y_taxi, int index) {

    if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y-1].num_taxi_presenti <
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y-1].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);
        pointer_map_sh->taxi_array[index].bloccato = 0;

        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].crossing_counter++;
        pointer_map_sh->taxi_array[index].max_time_request += pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].tempo_attraversamento;
        pointer_map_sh->taxi_array[index].distance_counter++;


        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti--;
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y - 1].num_taxi_presenti++;

        if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti > 1) {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y - 1].symbol_in_map = 'T';

        } else {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = '0';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y - 1].symbol_in_map = 'T';
        }


        pointer_map_sh->taxi_array[index].posizione_attuale.y--;

    } else if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y-1].num_taxi_presenti ==
               pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y-1].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);


        pointer_map_sh->taxi_array[index].bloccato = 1;

    }


}

void sopra(map* pointer_map_sh,int x_taxi, int y_taxi,int index) {

    if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x-1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti <
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x-1][pointer_map_sh->taxi_array[index].posizione_attuale.y].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);

        pointer_map_sh->taxi_array[index].bloccato = 0;

        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].crossing_counter++;
        pointer_map_sh->taxi_array[index].max_time_request += pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].tempo_attraversamento;
        pointer_map_sh->taxi_array[index].distance_counter++;


        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti--;
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x - 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti++;

        if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti > 1) {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x - 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';

        } else {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = '0';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x - 1][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
        }


        pointer_map_sh->taxi_array[index].posizione_attuale.x--;

    } else if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x-1][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti ==
               pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x-1][pointer_map_sh->taxi_array[index].posizione_attuale.y].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms,NULL);

        pointer_map_sh->taxi_array[index].bloccato = 1;

    }

}

void destra(map* pointer_map_sh,int x_taxi, int y_taxi, int index) {

    if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y+1].num_taxi_presenti <
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y+1].max_taxi_in_cella) {
        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms, NULL);

        pointer_map_sh->taxi_array[index].bloccato = 0;

        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].crossing_counter++;
        pointer_map_sh->taxi_array[index].max_time_request += pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].tempo_attraversamento;
        pointer_map_sh->taxi_array[index].distance_counter++;


        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti--;
        pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y + 1].num_taxi_presenti++;

        if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].num_taxi_presenti > 1) {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = 'T';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y + 1].symbol_in_map = 'T';

        } else {
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y].symbol_in_map = '0';
            pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y + 1].symbol_in_map = 'T';
        }


        pointer_map_sh->taxi_array[index].posizione_attuale.y++;

    } else if (pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y + 1].num_taxi_presenti ==
               pointer_map_sh->MAPPA[pointer_map_sh->taxi_array[index].posizione_attuale.x][pointer_map_sh->taxi_array[index].posizione_attuale.y + 1].max_taxi_in_cella) {

        tms.tv_sec = 0;
        tms.tv_nsec = pointer_map_sh->MAPPA[x_taxi][y_taxi].tempo_attraversamento;

        nanosleep(&tms, NULL);


        pointer_map_sh->taxi_array[index].bloccato = 1;



    }

}



#endif //PROGETTO_SO_CON_SIGNAL_HEADER_H
