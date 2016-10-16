#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "shared.h"

#define PATH "/home/gkarkow/trab-SO/"
#define MAXTF 20

int firstUsedPosition(SHM *mem) {
    int i;
    for (i=0; i<MAXTF; i++)
        if (mem->t[i].id != -1) break;
    return i;
}

int getSeconds(int h, int m) {
    return (h*3600 + m*60);
}

int calcTimeout(int h, int m) {
    time_t t;
    struct tm *now;

    int h_aux = h;
    int m_aux = m;

    time(&t);
    now = localtime(&t);
    int h_now = now->tm_hour;
    int m_now = now->tm_min;

    if (h_aux < h_now) h_aux += 24;

    if (h_aux > h_now && m_aux < m_now)
        h_aux = hsub(h_aux-1, h_now);
    else
        h_aux = hsub(h_aux, h_now);
    m_aux = msub(m_aux, m_now);

    return getSeconds(h_aux, m_aux);
}

int myTurn(int hsys, int msys, int hh, int mm) {
    return (hsys == hh && msys == mm);
}

int realTask(SHM *mem, int i) {
    return (mem->t[i].id >= 0);
}

void decrementTask(SHM *mem, int i) {
    int h, m, rest;

    rest = 0;
    h = mem->t[i].hh;
    m = mem->t[i].mm;

    if (mem->t[i].n == 1) {
        mem->t[i].id = -1;
        mem->n_tk -= 1;
    }
    else {
        mem->t[i].n -= 1;
        if (m + mem->t[i].mincr >= 60) rest = 1;
        mem->t[i].hh = hsum(h, mem->t[i].hincr + rest);
        mem->t[i].mm = msum(m, mem->t[i].mincr);
    }
}

void statisticHeader(FILE *st) {
    fprintf(st, "%20s\t%5s\t%5s\n", "tarefa", "t_ini", "t_fim");
    fflush(st);
}

void saveStatistic(FILE *st, char *task, struct tm *t1, struct tm *t2) {
    fprintf(st, "%20s\t%02d:%02d\t%02d:%02d\n", task, t1->tm_hour, t1->tm_min, t2->tm_hour, t2->tm_min);
    fflush(st);
}

int main(int argc, char *argv[]) {
    pid_t pid, sid;
    FILE *fp = NULL;
    FILE *st = NULL;

    int shmid = getshmID();
    SHM *mem = sharedMemory(shmid);

    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    umask(0);

    fp = fopen("log.txt", "w+");
    st = fopen("statistic.txt", "w+");

    sid = setsid();
    if (sid < 0) {
        fprintf(fp, "ERRO: setsid\n");
        fflush(fp);
        exit(1);
    }

    if ((chdir("/")) < 0) {
        fprintf(fp, "ERRO: chdir\n");
        fflush(fp);
        exit(1);
    }

    int i, h, haux, hsys, m, maux, msys;
    int s, ok, first_pos, first_turn, rest;
    int lower_timeout, semid, daemonid;
    char name[20];
    char task[50];
    time_t tempo;
    struct tm *tsys, *taux1, *taux2;
    struct timespec timeout;

    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;

    semid = getsemID();
    struct sembuf operation[2];

    daemonid = getpid();
    ok = 1;
    first_turn = 1;

    while (ok) {
        fprintf(fp, "roda_tarefa: PID %d\n", daemonid);
        fflush(fp);

        /* semaforo 1: espera semval == 0
         * (roda_tarefa: acordar) */
        operation[0].sem_num = 1;
        operation[0].sem_op = 0;
        operation[0].sem_flg = 0;

        if (mem->n_tk)
            semtimedop(semid, operation, 1, &timeout);
        else
            semop(semid, operation, 1);

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
            fprintf(fp, "ERRO: semop\n");
            fflush(fp);
            exit(1);
        }

        time(&tempo);
        tsys = localtime(&tempo);
        hsys = tsys->tm_hour;
        msys = tsys->tm_min;

        ok = semctl(semid, 2, GETVAL);

        if (mem->n_tk > 0 && ok) {
            first_pos = firstUsedPosition(mem);
            for (i=0; i<MAXTF; i++) {
                if (realTask(mem, i)) {
                    strcpy(task, PATH);
                    h = mem->t[i].hh;
                    m = mem->t[i].mm;

                    if (myTurn(hsys, msys, h, m)) {
                        if (first_turn) {
                            statisticHeader(st);
                            first_turn = 0;
                        }
                        strcpy(name, mem->t[i].arq);
                        strcat(task, name);
                        taux1 = localtime(&tempo);
                        pid = fork();
                        if (pid ==0) execve(task, NULL, NULL);
                        if (pid > 0) wait();
                        taux2 = localtime(&tempo);
                        saveStatistic(st, name, taux1, taux2);
                        decrementTask(mem, i);
                    }

                    s = calcTimeout(h, m);
                    if (i == first_pos) lower_timeout = s;
                    if (s < lower_timeout) lower_timeout = s;
                }
            }

            timeout.tv_sec = lower_timeout;
            ok = semctl(semid, 2, GETVAL);
        }

        /* semaforo 0: decrementa semval
         * (memoria compartilhada: liberada) */
        operation[0].sem_num = 0;
        operation[0].sem_op = -1;
        operation[0].sem_flg = 0;

        i = semop(semid, operation, 1);
        if (i == -1) {
            fprintf(fp, "ERRO: semop\n");
            fflush(fp);
            exit(1);
        }

        /* semaforo 1: se semval == 0
        /* (roda_tarefa: acordado por agenda_tarefa/fim_agenda) */
        if (!semctl(semid, 1, GETVAL)) {
            /* semaforo 1: incrementa semval
             * (roda_tarefa: dormir) */
            operation[0].sem_num = 1;
            operation[0].sem_op = 1;
            operation[0].sem_flg = 0;
            semop(semid, operation, 1);
        }
    }

    i = shmdt(mem);
    if (i == -1) {
        fprintf(fp, "ERRO: shmdt\n");
        fflush(fp);
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    fprintf(fp, "roda_tarefa: finalizado!\n");
    fflush(fp);

    fclose(fp);
    fclose(st);

    return 0;
}
