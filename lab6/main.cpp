#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define READERS 5
#define WRITERS 3

#define W_CYCLES 10
#define R_CYCLES 10

#define W_SLEEP 300
#define R_SLEEP 300
#define RAND_ADD_SLEEP 4000

#define MUTEX_ERROR -1
#define EVENT_ERROR -2
#define THREAD_ERROR -3

int sheared = 0;

HANDLE mutex;
HANDLE can_read;
HANDLE can_write;
LONG waiting_writers = 0;
LONG waiting_readers = 0;
LONG active_readers = 0;
bool active_writer = false;




void start_read(void)
{
    InterlockedIncrement(&waiting_readers);
    if (active_writer || waiting_writers)
        WaitForSingleObject(can_read, INFINITE);


    WaitForSingleObject(mutex, INFINITE);
    InterlockedIncrement(&active_readers);
    SetEvent(can_read);
    InterlockedDecrement(&waiting_readers);
    ReleaseMutex(mutex);

}

void stop_read(void)
{
    InterlockedDecrement(&active_readers);
    if (active_readers == 0)
    {
        ResetEvent(can_read);
        SetEvent(can_write);
    }
}

void start_write(void) {

    InterlockedIncrement(&waiting_writers);
    if (active_writer || active_readers > 0)  
        WaitForSingleObject(can_write, INFINITE);
    InterlockedDecrement(&waiting_writers);
    active_writer = true;
}

void stop_write(void) {
    active_writer = false;
    if (waiting_readers)
        SetEvent(can_read);
    else
        SetEvent(can_write);
}

DWORD WINAPI reader(CONST LPVOID lpParams) {
    int id = (long long)lpParams;
    srand(clock() + id);
    for (size_t i = 0; i < R_CYCLES; i++)
    {
        Sleep(R_SLEEP + rand() % RAND_ADD_SLEEP);
        start_read();
        printf("  Process reader ID = %d read:  %d\n", id, sheared);
        stop_read();
    }

    return 0;
}

DWORD WINAPI writer(CONST LPVOID lpParams)
{
    int id = (long long)lpParams;
    srand(time(NULL) + id + READERS);

    for (int i = 0; i < W_CYCLES; i++)
    {
        Sleep(W_SLEEP + rand() % RAND_ADD_SLEEP);
        start_write();
        sheared++;
        printf("  Process writer ID = %d write: %d \n", id, sheared);
        stop_write();
    }

    return 0;
}

int main(void) {
    setbuf(stdout, NULL);

    HANDLE readers[READERS];
    HANDLE writers[WRITERS];


    if (!(mutex = CreateMutex(NULL, FALSE, NULL)))
    {
        perror("Failed while CreateMutex");
        exit(MUTEX_ERROR);
    }


    if (!(can_read = CreateEvent(NULL, TRUE, FALSE, NULL)) || !(can_write = CreateEvent(NULL, FALSE, FALSE, NULL)))
    {
        perror("Failed while CreateEvent");
        exit(EVENT_ERROR);
    }


    for (int i = 0; i < READERS; i++)
        if (!(readers[i] = CreateThread(NULL, 0, reader, (LPVOID)(i+1), 0, NULL)))
        {
            perror("Failed while CreateThread");
            exit(THREAD_ERROR);
        }
    for (int i = 0; i < WRITERS; i++)
        if (!(writers[i] = CreateThread(NULL, 0, writer, (LPVOID)(i+1), 0, NULL)))
        {
            perror("Failed while CreateThread");
            exit(THREAD_ERROR);
        }


    WaitForMultipleObjects(READERS, readers, TRUE, INFINITE);
    WaitForMultipleObjects(WRITERS, writers, TRUE, INFINITE);

    CloseHandle(mutex);
    CloseHandle(can_read);
    CloseHandle(can_write);

    printf("Program terminated");

    return 0;
}
