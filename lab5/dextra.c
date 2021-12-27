#include <stdio.h>
#include "stdlib.h"
#include "time.h"
#include "sys/stat.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_AMOUNT 3

#define SEM_BIN 0
#define SEM_E 1
#define SEM_F 2

#define EMPTY_NUM 20

#define PRODUCE_NUM 3
#define CONSUME_NUM 5

#define CYCLES 5

#define SEM_ERROR -1
#define SEM_SET_ERR -2
#define SHM_ERROR -3
#define MEM_ERR -4
#define FORK_ERR -5
#define SEMOP_ERR -6

#define MAX_SEC 4

int *sharedMemoryPtr = NULL;
char *sharedCharMemoryPtr = NULL;

char alphabet = 'a';

struct sembuf prodStart[2] =
{
    {SEM_E, -1, 1},
    {SEM_BIN, -1, 1}
};
struct sembuf prodEnd[2] =
{
    {SEM_BIN, 1, 1},
    {SEM_F, 1, 1}
};
struct sembuf readStart[2] =
{
    {SEM_F, -1, 1},
    {SEM_BIN, -1, 1}
};
struct sembuf readEnd[2] =
{
    {SEM_BIN, 1, 1},
    {SEM_E, 1, 1}
};

void rand_sleep()
{
    sleep(rand() % MAX_SEC);
}

void producer(int semID, int prodID)
{
	for (int i = 0; i < CYCLES; i++)
	{
	rand_sleep();
	
    if (semop(semID, prodStart, 2) == -1)
    {
        perror("Producer semop error");
        exit(SEMOP_ERR);
    }

    sharedCharMemoryPtr[sharedMemoryPtr[0]] = sharedCharMemoryPtr[-1];

    printf("Producer PID = %d wrote %c\n", prodID, sharedCharMemoryPtr[-1] );
    sharedCharMemoryPtr[-1]++;
    sharedMemoryPtr[0]++;

    if (semop(semID, prodEnd, 2) == -1)
    {
        perror("Producer semop error");
        exit(SEMOP_ERR);
    }
	}
}

void consumer(int semID, int PID)
{
	for (int i = 0; i < 3; i++)
	{
    rand_sleep();
	
    if (semop(semID, readStart, 2) == -1)
    {
        perror("Consumer semop error");
        exit(SEMOP_ERR);
    }

    printf("Consumer PID = %d read %c\n", PID, sharedCharMemoryPtr[sharedMemoryPtr[1]]);
    sharedMemoryPtr[1]++;

    if (semop(semID, readEnd, 2) == -1)
    {
        perror("Consumer semop error");
        exit(SEMOP_ERR);
    }
	}
}

int create_sem()
{
    int semID = semget(IPC_PRIVATE, SEM_AMOUNT, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (semID == -1)
    {
        perror("Semaphore creation error.");
        exit(SEM_ERROR);
    }

    if (semctl(semID, SEM_BIN, SETVAL, 1) == -1 ||
        semctl(semID, SEM_E, SETVAL, EMPTY_NUM) == -1 ||
        semctl(semID, SEM_F, SETVAL, 0) == -1)
    {
        perror("Semaphore set error.");
        exit(SEM_SET_ERR);
    }
	return semID;
}

int create_seg()
{
    int shmID = shmget(IPC_PRIVATE, 2 * sizeof(int) + sizeof(char) + EMPTY_NUM * sizeof(char), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (shmID == -1)
    {
        perror("Shared memory creation error.");
        exit(SHM_ERROR);
    }


    sharedMemoryPtr = shmat(shmID, 0, 0);

    if (*sharedMemoryPtr == -1)
    {
        perror("Memory error.");
        exit(MEM_ERR);
    }
	return shmID;
}

int main()
{
    int semID = create_sem();

    int shmID = create_seg();

    sharedCharMemoryPtr = (char *)(sharedMemoryPtr + 2 * sizeof(int) + sizeof(char));
    sharedCharMemoryPtr[-1] = 'a';
    pid_t childID = -1;

    for (int i = 0; i < PRODUCE_NUM; i++)
    {
        if ((childID = fork()) == -1)
        {
            perror("fork error");
            exit(FORK_ERR);
        }
		
        if (childID == 0)
        {
            srand(time(NULL));
            producer(semID, getpid());
            exit(0);
        }
	}
	
	for (int i = 0; i < CONSUME_NUM; i++)
	{
        if ((childID = fork()) == -1)
        {
            perror("fork error");
            exit(FORK_ERR);
        }
		
        if (childID == 0)
        {
            srand(time(NULL));
            consumer(semID, getpid());
            exit(0);
        }
    }

    int status;
    for (int i = 0; i < 8; i++)
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
