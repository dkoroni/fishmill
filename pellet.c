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

#define SEM_MILL "/sem_mill"

key_t key;
int shmid;
void *shmPtr;
char (*millPtr)[11][11];

void termination(int sig){
    fprintf(stderr, "[ERROR] PID: %d Pellet forcefully interrupted!\n", getpid());
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

    int row = 0;
    int col = rand() % 11;
    sem_wait(sem);
    *(millPtr) [row][col] = 'o';
    sem_post(sem);
    sleep(1);
    while(1){
        // last row of the mill
        if (row == 10){
            sem_wait(sem);
            //printf("Child opening\n");
            *(millPtr)[row][col] = '.';
            //printf("Child closing\n");
            // TODO: signal back to swim_mill saying it fell off the mill
            printf("Pellet missed!\npid: %d\trow: %d col: %d\n", getpid(), row, col);
            sem_post(sem);
            break;
        }
        // row above fish
        else if(row == 9){
            sem_wait(sem);
            *(millPtr)[row][col] = '.';
            // check if there's a fish below
            if (*(millPtr)[row + 1][col] == 'F'){
                // TODO: signal back to swim_mill saying it was eaten
                row++;
                printf("Pellet eaten! YUM!\npid: %d\trow: %d col: %d\n", getpid(), row, col);
                sem_post(sem);
                break;
            }
            else{
                *(millPtr)[row + 1][col] = 'o';
            }
            sem_post(sem);
            row++;
        }
        else{
            sem_wait(sem);
            //printf("Child opening\n");
            *(millPtr)[row][col] = '.';
            *(millPtr)[row + 1][col] = 'o';
            //printf("Child closing\n");
            sem_post(sem);
            row++;
        }
        sleep(1);
    }
    sem_close(sem);
    shmctl(shmid, IPC_RMID, NULL);
}