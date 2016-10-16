#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include "shared.h"

#define PATH "/home/gkarkow/trab-SO/"
#define STAT "statistic.txt"
#define MAXLINE 100

void printStatistic(char *file) {
    char str[MAXLINE];
    FILE *st = NULL;
    int blank = 1;

    st = fopen(file, "r");
    if (!st) {
        perror("fopen");
        exit(1);
    }

    printf("Estatistica:\n");
    while (fgets(str, MAXLINE, st) != NULL) {
        if (str) blank = 0;
        printf("%s", str);
    }

    if (blank) printf("Nenhuma tarefa foi executada.\n");
    printf("\n");

    fclose(st);
}

int main(int argc, char *argv[]) {
    int i, shmid, semid;
    char file[50];

    shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    semid = getsemID();
    struct sembuf operation[2];

    if (mem->n_tk) {
        strcpy(file, PATH);
        strcat(file, "lista_tarefa");
        printf("As seguintes tarefas nao serao executadas:\n");
        if (fork()==0) execve(file, NULL, NULL);
        wait();
    }

    /* semaforo 2: decrementa semval
     * (roda_tarefa: interromper loop) */
    operation[0].sem_num = 2;
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

    printf("Pressione 'Enter' para finalizar e ");
    printf("exibir dados de execucao das tarefas.\n");
    fflush(stdin);
    getchar();

    strcpy(file, PATH);
    strcat(file, STAT);
    printStatistic(file);

    i = shmdt(mem);
    if (i == -1) {
        perror("shmdt");
        exit(1);
    }

    i = semctl(semid, 1, IPC_RMID);
    if (i == -1) {
        perror("semctl");
        exit(1);
    }

    i = shmctl(shmid, IPC_RMID, NULL);
    if (i == -1) {
        perror("shmctl");
        exit(1);
    }

    printf("Agenda encerrada.\n");

    return 0;
}
