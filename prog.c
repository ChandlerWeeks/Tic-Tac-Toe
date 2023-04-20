#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>

// Data structure for shared memory
typedef struct {
    int turn;
    int board[3][3];
} gameData;

// Semaphore operations
struct sembuf reserve = {0, -1, 0};
struct sembuf release = {0, 1, 0};

// Function prototypes
void player1Loop(int semid, gameData *data);
void player2Loop(int semid, gameData *data);
void displayBoard(gameData *data);
int checkWin(gameData *data);
void makeMove(gameData *data, int player);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s -1 | -2\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int player = atoi(argv[1]);
    if (player != -1 && player != -2) {
        printf("Invalid player number. Choose -1 or -2\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    const char *fifoName = "xoSync";
    mkfifo(fifoName, 0666); // Create the FIFO

    key_t shmKey, semKey;
    int shmid, semid;
    gameData *data;

    if (player == -1) {
        // Player 1 steps

        // Generate two random numbers for ftok
        int rand1 = rand();
        int rand2 = rand();

        // Generate System V keys
        shmKey = ftok(fifoName, rand1);
        semKey = ftok(fifoName, rand2);

        // Create shared memory and semaphore set
        shmid = shmget(shmKey, sizeof(gameData), IPC_CREAT | 0666);
        semid = semget(semKey, 2, IPC_CREAT | 0666);

        // Initialize semaphores
        semctl(semid, 0, SETVAL, 1); // Player 1 semaphore (available)
        semctl(semid, 1, SETVAL, 0); // Player 2 semaphore (in use)

        // Attach and initialize shared memory
        data = (gameData *)shmat(shmid, NULL, 0);
        data->turn = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                data->board[i][j] = 0;
            }
        }

        // Write random numbers to FIFO
        int fifoFd = open(fifoName, O_WRONLY);
        write(fifoFd, &rand1, sizeof(int));
        write(fifoFd, &rand2, sizeof(int));
        close(fifoFd);

        // Enter game play loop
        player1Loop(semid, data);

        // Cleanup
        open(fifoName, O_WRONLY);
        close(fifoFd);
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
    } 
    
    else {
        // Player 2 steps

        // Read random numbers from FIFO
        int rand1, rand2;
        int fifoFd = open(fifoName, O_RDONLY);
        read(fifoFd, &rand1, sizeof(int));
        read(fifoFd, &rand2, sizeof(int));
        close(fifoFd);

        // Generate System V keys
        shmKey = ftok(fifoName, rand1);
        semKey = ftok(fifoName, rand2);

        // Retrieve shared memory and semaphore set
        shmid = shmget(shmKey, sizeof(gameData), 0666);
        semid = semget(semKey, 2, 0666);

        // Attach shared memory
        data = (gameData *)shmat(shmid, NULL, 0);

        // Enter game play loop
        player2Loop(semid, data);

        // Cleanup
        open(fifoName, O_RDONLY);
        close(fifoFd);
        shmdt(data);
    }

    exit(EXIT_SUCCESS);
}

void player1Loop(int semid, gameData *data) {
    while (data->turn >= 0) {
        reserve.sem_num = 0;
        semop(semid, &reserve, 1); // Reserve player 1's semaphore

        displayBoard(data);
        makeMove(data, 1);
        displayBoard(data);

        if (checkWin(data) || data->turn >= 9) {
            data->turn = -1;
        }

        release.sem_num = 1;
        semop(semid, &release, 1); // Release player 2's semaphore
    }
}

void player2Loop(int semid, gameData *data) {
    while (1) {
        reserve.sem_num = 1;
        semop(semid, &reserve, 1); // Reserve player 2's semaphore

        displayBoard(data);

        if (data->turn == -1) {
            break;
        }

        makeMove(data, -1);
        displayBoard(data);

        data->turn++;

        release.sem_num = 0;
        semop(semid, &release, 1); // Release player 1's semaphore
    }
}

void displayBoard(gameData *data) {
    printf("\nCurrent Board:\n");
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

int checkWin(gameData *data) {
    int sum;

    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        sum = data->board[i][0] + data->board[i][1] + data->board[i][2];
        if (abs(sum) == 3) {
            return sum / 3;
        }

        sum = data->board[0][i] + data->board[1][i] + data->board[2][i];
        if (abs(sum) == 3) {
            return sum / 3;
        }
    }

    // Check diagonals
    sum = data->board[0][0] + data->board[1][1] + data->board[2][2];
    if (abs(sum) == 3) {
        return sum / 3;
    }

    sum = data->board[0][2] + data->board[1][1] + data->board[2][0];
    if (abs(sum) == 3) {
        return sum / 3;
    }

    return 0;
}


/* int checkWin(gameData *data) { //TODO: check these return statements
    int sum;

    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        sum = data->board[i][0] + data->board[i][1] + data->board[i][2];
        if (abs(sum) == 3) {
            exit(EXIT_FAILURE);
        }

        sum = data->board[0][i] + data->board[1][i] + data->board[2][i];
        if (abs(sum) == 3) {
            exit(EXIT_FAILURE);
        }
    }

    // Check diagonals
    sum = data->board[0][0] + data->board[1][1] + data->board[2][2];
    if (abs(sum) == 3) {
        exit(EXIT_FAILURE);
    }

    sum = data->board[0][2] + data->board[1][1] + data->board[2][0];
    if (abs(sum) == 3) {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
} */

void makeMove(gameData *data, int player) {
    int row, col;
    row = rand() % 3;
    col = rand() % 3;

    while (data->board[row][col] != 0) {
        row = rand() % 3;
        col = rand() % 3;
    }

    data->board[row][col] = player;
}
