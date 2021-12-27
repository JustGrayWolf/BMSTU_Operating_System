#include <stdio.h>
#include "stdlib.h"
#include "time.h"
#include "sys/stat.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_AMOUNT 5

#define MAX_SEC 4

#define CYCLES 10

#define WRITERS_AMOUNT 3
#define READERS_AMOUNT 5

#define ACTIVE_READERS 0
#define ACTIVE_WRITERS 1
#define WAIT_READERS 2
#define WAIT_WRITERS 3
#define BIN_SEM 4

#define SEM_ERROR -1
#define SEM_SET_ERR -2
#define SHM_ERR -3
#define MEM_ERR -4
#define FORK_ERR -5
#define SEMOP_ERR -6

struct sembuf start_state[] =
{
     { BIN_SEM, 1, 0}
};

struct sembuf startWrite[] =
{
    { WAIT_WRITERS, 1, 0 },
    { ACTIVE_READERS, 0, 0 },
    { ACTIVE_WRITERS, 0, 0 },
    { BIN_SEM, -1, 0},
    { ACTIVE_WRITERS, 1, 0 },
    { WAIT_WRITERS, -1, 0 }
};

struct sembuf stopWrite[] =
{
    { ACTIVE_WRITERS, -1, 0 },
    { BIN_SEM, 1, 0}
};

struct sembuf startRead[] =
{
    { WAIT_READERS, 1, 0 },
    { WAIT_WRITERS, 0, 0 },
    { ACTIVE_WRITERS,  0, 0 },
    { ACTIVE_READERS, 1, 0 },
    { WAIT_READERS, -1, 0 }
};

struct sembuf stopRead[] =
{
    { ACTIVE_READERS, -1, 0 }
};

int *sharedMemoryPtr = NULL;

void rand_sleep()
{
    sleep(rand() % MAX_SEC);
}

void writer(int semID, int writerID)
{
    rand_sleep();
    if (semop(semID, startWrite, 6) == -1)
    {
        perror("Semop error");
        exit(SEMOP_ERR);
    }

    (*sharedMemoryPtr)++;
    printf("Writer PID = %d write value %d\n", writerID, *sharedMemoryPtr);

    if (semop(semID, stopWrite, 2) == -1)
    {
        perror("semop error");
        exit(SEMOP_ERR);
    }
}

void reader(int semID, int readerID)
{
    rand_sleep();
    if (semop(semID, startRead, 5) == -1)
    {
        perror("Semop error");
        exit(SEMOP_ERR);
    }

    printf("Reader PID = %d reads value %d\n", readerID, *sharedMemoryPtr);

    if (semop(semID, stopRead, 1) == -1)
    {
        perror("Writer semop error");
        exit(SEMOP_ERR);
    }
}

int main()
{
    srand(time(NULL));
    int semID = semget(IPC_PRIVATE, SEM_AMOUNT, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (semID == -1)
    {
        perror("Semaphore creation error.");
        exit(SEM_ERROR);
    }

    if (semctl(semID, 2, SETVAL, 1) == -1)
    {
        perror("Semaphore set error.");
        exit(SEM_SET_ERR);
    }

    semop(semID, start_state, 1);

    int shmID = shmget(IPC_PRIVATE, sizeof(int), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (shmID == -1)
    {
        perror("Shared memory creation error.");
        exit(SHM_ERR);
    }

    sharedMemoryPtr = shmat(shmID, 0, 0);
    if (*sharedMemoryPtr == -1)
    {
        perror("Memory all error.");
        exit(MEM_ERR);
    }

    pid_t childID = -1;
    for (int i = 0; i < WRITERS_AMOUNT; i++)
    {
        if ((childID = fork()) == -1)
        {
            perror("Write fork error");
            exit(FORK_ERR);
        }
        if (childID == 0)
        {
            srand(time(NULL));
            for (int i = 0;i < CYCLES; i++)
                writer(semID, i);
            exit(0);
        }
    }

    for (int i = 0; i < READERS_AMOUNT; i++)
    {
        if ((childID = fork()) == -1)
        {
            perror("Reader fork error");
            exit(FORK_ERR);
        }
        if (childID == 0)
        {
            srand(time(NULL));
            for (int i = 0;i < CYCLES; i++)
                reader(semID, i);
            exit(0);
        }
    }

    int status;
    for (int i = 0; i < WRITERS_AMOUNT + READERS_AMOUNT; i++)
        wait(&status);

    if (shmdt(sharedMemoryPtr) == -1)
    {
        perror("SHMDT error");
        exit(MEM_ERR);
    }

    if (shmctl(shmID, IPC_RMID, NULL) == -1)
    {
        perror("SHMCTL error");
        exit(MEM_ERR);
    }
    printf("\n\nProgram terminated\n");
    exit(0);
}
