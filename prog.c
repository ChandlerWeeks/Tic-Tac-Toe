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

// display the contents of the board
void displayBoard(struct board *data) {
    printf("\nCurrent Board:\n");
    // loop through each index of the table, and output the value
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (data->board[i][j] == 1) {
                printf("X ");
            } 
            else if (data->board[i][j] == -1) {
                printf("O ");
            } 
            else {
                printf("- ");
            }
        }
        printf("\n");
    }
}

/* allows a player to change the value on a table by generating
a random pairing until one is valid */
void makeMove(struct board *data, int player) {
    int row, col;
    row = rand() % 3;
    col = rand() % 3;

    while (data->board[row][col] != 0) {
        row = rand() % 3;
        col = rand() % 3;
    }

    data->board[row][col] = player;
}

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

// set a new semaphores value to available
int initSemAvailable(int semId, int semNum)
{
  union semun arg;
  arg.val = 1;
  return semctl(semId, semNum, SETVAL, arg);
}

// set a new semaphores value to in use
int initSemInUse(int semId, int semNum)
{
  union semun arg;
  arg.val = 0;
  return semctl(semId, semNum, SETVAL, arg);
}

// set an existing semaphores value to in use
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

// set an existing semaphores value to available
int releaseSem(int semId, int semNum)
{
  struct sembuf sops;

  sops.sem_num = semNum;
  sops.sem_op = 1;
  sops.sem_flg = bsUseSemUndo ? SEM_UNDO : 0;

  return semop(semId, &sops, 1);
}

// some setup for player 1
void player1_setup() {
    int rand1, rand2, shm, sem, fd; // random values storage, shm and semaphore storage
    // attempt to create a FIFO named xoSync
    checkError(mkfifo("xoSync", S_IRWXU), "mkfifo");

    // generate two random numbers
    rand1 = rand();
    rand2 = rand();

    // generate system V keys
    key_t shmKey = ftok("xoSync", rand1);
    key_t semKey = ftok("xoSync", rand2);

    // use the keys as projection values for shared memory/semaphores
    shm = checkError(shmget(shmKey, sizeof(struct board), IPC_CREAT | 8888), "shmget");
    sem = checkError(semget(semKey, 2, IPC_CREAT | 4444), "semget");

    // shared memory of board struct
    struct board* boardPtr = checkError(shmat(shm, NULL, 0), "shmat");

    // initialize the semaphores in the set
    initSemAvailable(sem, 0); // player 1 semaphore
    initSemInUse(sem, 1); // player 2 semaphore

    // empty board and set turn counter to 0
    boardPtr->turn = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            boardPtr->board[i][j] = 0;
        }
    }

    // open fifo, write the 2 random values, and close the fifo
    fd = open("xoSync", O_WRONLY);
    checkError(write(fd, &rand1, sizeof(int)), "write");
    checkError(write(fd, &rand2, sizeof(int)), "write");
    close(fd);

    // enter gameplay loop

    // open fifo for write, and close it for cleanup
    fd = open("xoSync", O_WRONLY);
    close(fd);
    // detactch shared memory
    shmdt(boardPtr);

    // deleted shared memory and semaphores
    shmctl(shm, IPC_RMID, NULL);
    semctl(sem, 0, IPC_RMID);
}

void player2_setup() {
    int rand1, rand2, fd, shm, sem;

    // open and read two random integers from the FIFO, and close the fifo
    fd = checkError(open("xoSync", O_RDONLY), "open");
    checkError(read(fd, &rand1, sizeof(int)), "read");
    checkError(read(fd, &rand2, sizeof(int)), "read");
    close(fd);

    // Generate System V keys
    key_t shmKey = ftok("xoSync", rand1);
    key_t semKey = ftok("xoSync", rand2);

    // retreieve shared memory and semaphore
    shm = checkError(shmget(shmKey, sizeof(struct board), 8888), "shmget");
    sem = checkError(semget(semKey, 2, 4444), "semget");

    // attack shared memory
    struct board* boardPtr = checkError(shmat(shm, NULL, 0), "shmat");

    // Enter game play loop


    // open fifo for read, close the fifo
    fd = open("xoSync", O_RDONLY);
    close(fd);
    // detach the segment of shared memory
    shmdt(boardPtr);
}

void player1_loop()
{
  
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