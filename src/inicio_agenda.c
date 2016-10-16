#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include "shared.h"

#define PATH "/home/gkarkow/trab-SO/"
#define DAEMON "roda_tarefa"
#define MAXTF 20
#define NSEM 3

int main(int argc, char *argv[]) {
    int i, semid, shmid;
    char daemon[50];

    semid = getsemID();
    struct sembuf operation[2];

    short sem[NSEM];
    sem[0] = 0;  /* memoria compartilhada: liberada */
    sem[1] = 1;  /* roda_tarefa: dormir */
    sem[2] = 1;  /* roda_tarefa: loop ativo */

    i = semctl(semid, 0, SETALL, sem);
    if (i == -1) {
        perror("semctl");
        exit(1);
    }

    shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    /* contador de tarefas agendadas */
    mem->n_tk = 0;
    /* contador que gera id das tarefas: iniciado em 0 */
    mem->tk_id = 0;
    /* vetor iniciado com todas as posicoes 'livres' */
    for (i=0; i<MAXTF; i++) mem->t[i].id = -1;

    strcpy(daemon, PATH);
    strcat(daemon, DAEMON);
    if (fork()==0) execve(daemon, NULL, NULL);

    i = shmdt(mem);
    if (i == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
