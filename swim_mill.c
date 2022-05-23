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
#include <time.h>
#include <signal.h>

#define SEM_MILL "/sem_mill"

key_t key;
int shmid;
void *shmPtr;
char (*millPtr)[11][11];

void termination(int sig){
    kill(0, SIGINT);
    shmctl(shmid, IPC_RMID, NULL);
    sleep(2);
    printf("Interrupted all child processes\n");
    exit(0);
}

int main(int argc, char* argv[]){
    signal(SIGINT, termination);
    sem_unlink(SEM_MILL);
    
    pid_t pid;
    char mill[11][11];

    sem_t *sem = NULL;
    sem = sem_open(SEM_MILL, O_CREAT | O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO, 1);
    
    // get shared memory allocated
    shmid = shmget(key, sizeof(char[11][11]), 0666 | IPC_CREAT);
    if (shmid < 0){
        perror("shmget");
        exit(1);
    }

    // assign array pointer to shm
    millPtr = shmat(shmid, NULL, 0);
    if ((millPtr) == (void*) - 1){
        perror("shmat");
        exit(1);
    }

    // populate swim mill with '.'
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++){
            *(millPtr)[i][j] = '.';
            //printf("%c ", *(millPtr+i)[j]);
        }
        //printf("\n");
    }

    // shmid as string
    char shmidStr[50];
    snprintf(shmidStr, sizeof(shmidStr), "%d", shmid);

    // change env to store shmid as string
    setenv("SHM_ENV_VAR", shmidStr, 1);

    time_t my_t, fire_t;
    int count = 0;
    int row = 0;
    int col = 2;
    int nextPellet = 1;
    my_t = time(NULL);
    fire_t = my_t + 30;
    while (my_t < fire_t){
        my_t = time(NULL);
        //create child process
        if (count == 1){
            pid = fork();
            if (pid == 0){
                static char *args[] = {"./fish", NULL};
                execv(args[0], args);
            }
            else if(pid < 0){
                printf("Failed to fork\n");
            }
        }
        if (count % nextPellet == 0){
            nextPellet = rand() % 10 + 5;
            pid = fork();
            if (pid == 0){
                static char *args[] = {"./pellet", NULL};
                execv(args[0], args);
            }
            else if(pid < 0){
                printf("Failed to fork\n");
            }
        }
        sem_wait(sem);
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < 11; j++){
                mill[i][j] = *(millPtr)[i][j];
            }
        }
        sem_post(sem);
        printf("______________________\n");
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < 11; j++){
                printf("%c ", mill[i][j]);
            }
            printf("\n");
        }
        printf("______________________\n");
        usleep(250000);
        count++;
    }
    sem_close(sem);

    millPtr = NULL;
    sem_unlink(SEM_MILL);
    shmctl(shmid, IPC_RMID, NULL);
    kill(0, SIGTERM);
    return 0;
}
