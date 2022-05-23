#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <math.h>

#define SEM_MILL "/sem_mill"

key_t key;
int shmid;
void *shmPtr;
char (*millPtr)[11][11];

void termination(int sig){
    fprintf(stderr, "[ERROR] PID: %d Fish forcefully interrupted!\n", getpid());
    shmctl(shmid, IPC_RMID, NULL);
    exit(1);
}

int main(int argc, char* argv[]){
    signal(SIGINT, termination);

    srand(time(NULL));
    sem_t *sem = NULL;
    sem = sem_open(SEM_MILL, O_RDWR);

    // get shmid
    shmid = atoi(getenv("SHM_ENV_VAR"));
    // assign array pointer to shm
    millPtr = shmat(shmid, NULL, 0);
    if ((millPtr) == (void*) - 1){
        perror("shmat");
        exit(1);
    }

    float distance = 0;
    float currDist = 100;
    int move = 0;
    bool hasPellet = 0;
    int row = 10;
    int col = 5;

    sem_wait(sem);
    *(millPtr) [row][col] = 'F';
    sem_post(sem);
    while(1){
        distance = 0;
        currDist = 100;
        move = 0;
        hasPellet = 0;
        sem_wait(sem);
        for (int i = 0; i < 11; i++){
            for (int j = 0; j < 11; j++){
                if (*(millPtr)[i][j] == 'o'){
                    int y = 10 - i;
                    int x = col - j;
                    x = abs(x);
                    // skip pellet if its not possible to reach
                    if (y < x)
                        continue;
                    hasPellet = 1;
                    distance = sqrt(pow(i - 11, 2) + pow(j - col, 2) * 1.0);
                    //printf("distance: %f", distance);
                    if (distance < currDist){
                        currDist = distance;
                        if (j < col)
                            move = -1;
                        else if (j > col)
                            move = 1;
                        else
                            move = 0;
                    }  
                }
            }
        }
        *(millPtr)[10][col] = '.';
        *(millPtr)[10][col+move] = 'F';
        col = col + move;
        if (hasPellet == 0){
            if (col > 5){
                move = -1;
                *(millPtr)[10][col] = '.';
                *(millPtr)[10][col+move] = 'F';
            }
            else if (col < 5){
                move = 1;
                *(millPtr)[10][col] = '.';
                *(millPtr)[10][col+move] = 'F';
            }
        }
        sem_post(sem);
        sleep(1);
    }
    sem_close(sem);
    shmctl(shmid, IPC_RMID, NULL);
}