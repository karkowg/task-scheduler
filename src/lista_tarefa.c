#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include "shared.h"

#define MAXTF 20

int main(int argc, char *argv[]) {
    int i, shmid, semid;

    shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    semid = getsemID();
    struct sembuf operation[2];

    if (mem->n_tk == 0) {
        printf("Nenhuma tarefa agendada.\n");
        exit(0);
    }

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

    header();
    for (i=0; i<MAXTF; i++)
        if (mem->t[i].id != -1) printTask(mem->t[i]);
    printf("\n");

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

    i = shmdt(mem);
    if (i == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
