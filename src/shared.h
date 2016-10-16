typedef struct task {
    int id;
    int hh;
    int mm;
    int n;
    int hincr;
    int mincr;
    char arq[20];
} Task;

typedef struct shm {
    int tk_id;   /* contador que gera id das tarefas */
    int n_tk;    /* contador de tarefas agendadas */
    Task t[20];  /* vetor de tarefas agendadas */
} SHM;

void header();
void printTask(Task tk);
int hsum(int h1, int h2);
int msum(int m1, int m2);
int hsub(int h1, int h0);
int msub(int m1, int m0);
int getsemID();
int getshmID();
SHM * sharedMemory(int shmid);
