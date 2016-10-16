#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "shared.h"

#define SHMKEYPATH "/dev/null"
#define SHMKEYID 1
#define SHMSIZE sizeof(SHM)
#define SEMKEYPATH "/dev/null"
#define SEMKEYID 1
#define NSEM 3

    /* semaforo 0: memoria compartilhada
     *     semval == 0: liberada
     *     semval == 1: em uso */

    /* semaforo 1: roda_tarefa status
     *     semval == 0: acordar
     *     semval == 1: dormir */

    /* semaforo 2: roda_tarefa loop
     *     semval == 0: interromper
     *     semval == 1: ativar */

void header() {
    printf("\n");
    printf("%6s\t", "tarefa");
    printf("%20s\t", "arq_tarefa");
    printf("hh:mm\t");
    printf("%2s\t", "n");
    printf("%5s\t", "incr");
    printf("\n");
}

void printTask(Task tk) {
    printf("%6d\t", tk.id);
    printf("%20s\t", tk.arq);
    printf("%02d:%02d\t", tk.hh, tk.mm);
    printf("%02d\t", tk.n);
    if (tk.n == 1) printf("%5s\t", "-");
    else printf("%02d:%02d\t", tk.hincr, tk.mincr);
    printf("\n");
}

int hsum(int h1, int h2) {
    int hh = h1 + h2;
    if (hh >= 24) hh -= 24;
    return hh;
}

int msum(int m1, int m2) {
    int mm = m1 + m2;
    if (mm >= 60) mm -= 60;
    return mm;
}

int hsub(int h1, int h0) {
    return (h1 - h0);
}

int msub(int m1, int m0) {
    int m = m1;
    if (m < m0) m += 60;
    return (m - m0);
}

int getsemID() {
    key_t semkey;
    int semid;
    struct sembuf operation[2];
    short sem[NSEM];

    semkey = ftok(SEMKEYPATH,SEMKEYID);
    if (semkey == -1) {
        perror("ftok");
        exit(1);
    }

    semid = semget(semkey, NSEM, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    return semid;
}

int getshmID() {
    key_t shmkey;
    int shmid;

    shmkey = ftok(SHMKEYPATH, SHMKEYID);
    if (shmkey == -1) {
        perror("ftok");
        exit(1);
    }

    shmid = shmget(shmkey, SHMSIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    return shmid;
}

SHM * sharedMemory(int shmid) {
    SHM *mem;
    mem = shmat(shmid, (void *)0, 0);
    if (mem == (SHM *)(-1)) {
        perror("shmat");
        exit(1);
    }
    return mem;
}
