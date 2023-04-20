#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

struct board {
    int turn;
    int board[3][3];
};

int checkError(int val, char *msg)
{
    if (val == -1)
    {
        if (errno == EEXIST) return val;
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}

int bsUseSemUndo = 0;
int bsRetryOnEintr = 1;

int initSemAvailable(int semId, int semNum)
{
  union semun arg;
  arg.val = 1;
  return semctl(semId, semNum, SETVAL, arg);
}

int initSemInUse(int semId, int semNum)
{
  union semun arg;
  arg.val = 0;
  return semctl(semId, semNum, SETVAL, arg);
}

int reserveSem(int semId, int semNum)
{
  struct sembuf sops;

  sops.sem_num = semNum;
  sops.sem_op = -1;
  sops.sem_flg = bsUseSemUndo ? SEM_UNDO : 0;

  while (semop(semId, &sops, 1) == -1)
    {
      if (errno != EINTR || !bsRetryOnEintr)
	return -1;
    }

  return 0;
}

int releaseSem(int semId, int semNum)
{
  struct sembuf sops;

  sops.sem_num = semNum;
  sops.sem_op = 1;
  sops.sem_flg = bsUseSemUndo ? SEM_UNDO : 0;

  return semop(semId, &sops, 1);
}

void player1_setup() {
    int rand1, rand2, shm, sem; // random values storage, shm and semaphore storage
    // attempt to create a FIFO named xoSync
    checkError(mkfifo("xoSync", S_IRWXU), "mkfifo");

    rand1 = rand();
    rand2 = rand();

    key_t shmKey = ftok("xoSync", rand1);
    key_t semKey = ftok("xoSync", rand2);

    shm = checkError(shmget(shmKey, sizeof(struct board), IPC_CREAT | 8888), "shmget");
    sem = checkError(semget(semKey, 2, IPC_CREAT | 4444), "semget");

    struct board* boardPtr = checkError(shmat(shm, NULL, 0), "shmat");

    // empty board and set turn counter to 0
    boardPtr->turn = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            boardPtr->board[i][j] = 0;
        }
    }

    // initialize the semaphores in the set
    semArgs.val = 1;
    semctl(sem, 0, SETVAL, semArgs); // player 1 semaphore is available
    semArgs.val = 0;
    semctl(sem, 1, SETVAL, semArgs); // player 2 semaphore is in use

    shmdt(boardPtr);
}

void player2_setup() {

}


int main(int argc, char *argv[])
{
    int player; 
    union semun semArgs;
    struct sembuf semOps;

    // seed rng
    srand(time(0));

    // check and validate CLI arguements
    if (argc != 2)
    {
        checkError(-1, "Arguements");
    }
    player = atoi(argv[1]);

    if (player != -1 || player != -2) {
        checkError(-1, "Player Number");
    }

    exit(EXIT_SUCCESS);
}