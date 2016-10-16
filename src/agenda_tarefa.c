#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <time.h>
#include <unistd.h>
#include "shared.h"

#define MAXTF 20

int argtoint(char *arg, char t) {
    int n;
    switch (t) {
        case 'h': n = (arg[0]-'0')*10 + (arg[1]-'0');
            break;
        case 'm': n = (arg[3]-'0')*10 + (arg[4]-'0');
            break;
    }
    return n;
}

void scheduled(Task tk) {
    printf("A seguinte tarefa foi agendada:\n");
    header();
    printTask(tk);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc!=4) {
        fprintf(stderr, "%s: numero de argumentos invalido.\n", argv[0]);
        fprintf(stderr, "Sintaxe correta: agenda_tarefa hh:mm n arq_tarefa\n");
        exit(1);
    }

    int i, shmid, semid;
    time_t tempo;
    struct tm *tsys;

    shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    semid = getsemID();
    struct sembuf operation[2];

    if (mem->n_tk == MAXTF) {
        printf("ERRO: Agenda cheia!\n");
        exit(1);
    }

    time(&tempo);
    tsys = localtime(&tempo);

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

    int rest = 0;

    for (i=0; i<MAXTF; i++) {
        if (mem->t[i].id == -1) {
            mem->t[i].id = mem->tk_id;
            mem->t[i].hincr = argtoint(argv[1], 'h');
            mem->t[i].mincr = argtoint(argv[1], 'm');
            if (tsys->tm_min + mem->t[i].mincr >= 60) rest = 1;
            mem->t[i].hh = hsum(tsys->tm_hour, mem->t[i].hincr + rest);
            mem->t[i].mm = msum(tsys->tm_min, mem->t[i].mincr);
            mem->t[i].n = atoi(argv[2]);
            strcpy(mem->t[i].arq, argv[3]);
            mem->n_tk += 1;
            mem->tk_id += 1;
            break;
        }
    }

    scheduled(mem->t[i]);

    /* semaforo 0: decrementa semval
     * (memoria compartilhada: liberada) */
    operation[0].sem_num = 0;
    operation[0].sem_op = -1;
    operation[0].sem_flg = 0;

    /* semaforo 1: decrementa semval
     * (roda_tarefa: acordar) */
    operation[1].sem_num = 1;
    operation[1].sem_op = -1;
    operation[1].sem_flg = 0;

    i = semop(semid, operation, 2);
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
