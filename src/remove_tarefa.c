#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include "shared.h"

#define MAXTF 20

int main(int argc, char *argv[]) {
    if (argc!=2) {
        fprintf(stderr, "%s: numero de argumentos invalido.\n", argv[0]);
        return 1;
    }

    int i, id, shmid, semid;

    shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    semid = getsemID();
    struct sembuf operation[2];

    id = atoi(argv[1]);

    /* semaforo 0: espera semval == 0 */
    operation[0].sem_num = 0;
    operation[0].sem_op = 0;
    operation[0].sem_flg = 0;

    /* semaforo 0: incrementa semval
     * (memoria compartilhada: em uso) */
    operation[1].sem_num = 0;
    operation[1].sem_op = 1;
    operation[1].sem_flg = 0;

    i = semop(semid, operation, 2);
    if (i == -1) {
        perror("semop");
        exit(1);
    }

    int removed = 0;
    for (i=0; i<MAXTF; i++) {
        if (id == mem->t[i].id) {
            mem->t[i].id = -1;
            mem->n_tk -= 1;
            removed = 1;
            break;
        }
    }

    /* semaforo 0: decrementa semval
     * (memoria compartilhada: liberada) */
    operation[0].sem_num = 0;
    operation[0].sem_op = -1;
    operation[0].sem_flg = 0;

    i = semop(semid, operation, 1);
    if (i == -1) {
        perror("semop");
        exit(1);
    }

    if (removed) printf("Tarefa %d removida.\n", id);
    else printf("A tarefa %d nao encontra-se nessa agenda.\n", id);

    i = shmdt(mem);
    if (i == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
