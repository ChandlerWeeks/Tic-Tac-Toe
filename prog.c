#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/stat.h>

union semun
{

};

struct board {
    int turn;
    int board[3][3];
};

int checkError(int val, char *msg)
{
    if (val == -1)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return val;
}

void player1_loop() {

}

int main(int argc, char *argv[])
{
    int player; 
    union semun semArgs;
    struct sembuf semOps;
    if (argc != 2)
    {
        checkError(-1, "Arguements");
    }
    player = atoi(argv[1]);

    if (player != -1 || player != -2) {
        checkError(-1, "Player Number");
    }

    checkError(mkfifo("xoSync", S_IRWXU), "mkfifo");

    exit(EXIT_SUCCESS);
}