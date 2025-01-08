#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_READERS 10
#define MAX_WRITERS 5
#define RESOURCE_USE_TIME 2000 
#define DELAY_TIME 1000        

const char* FILENAME = "shared_resource.txt"; 
HANDLE resourceMutex;    
HANDLE readerCountMutex; 
int readerCount = 0;     
double totalReaderWaitTime = 0.0;
double totalWriterWaitTime = 0.0;
int readerOps = 0;
int writerOps = 0;

CRITICAL_SECTION logLock;

void log_message(const char* role, int id, const char* action) {
    EnterCriticalSection(&logLock);
    printf("[%s %d] %s\n", role, id, action);
    LeaveCriticalSection(&logLock);
}

void simulate_delay(int minTime, int maxTime) {
    Sleep((rand() % (maxTime - minTime + 1)) + minTime);
}


void read_file(int readerId) {
    FILE* file = nullptr;

    if (fopen_s(&file, FILENAME, "r") == 0 && file != nullptr) {
        char buffer[256];
        size_t totalChars = 0;

        while (fgets(buffer, sizeof(buffer), file)) {
            totalChars += strlen(buffer);
            simulate_delay(100, 200);
        }

        fclose(file);
        log_message("Reader", readerId, "read from file");
    }
    else {
        log_message("Reader", readerId, "failed to open file for reading");
        printf("[Reader %d] Error opening file for reading\n", readerId);
    }
}

void write_file(int writerId) {
    FILE* file = nullptr;

    if (fopen_s(&file, FILENAME, "a") == 0 && file != nullptr) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "[Writer %d] wrote data at %llu\n", writerId, GetTickCount64());

        if (fputs(buffer, file) != EOF) {
            log_message("Writer", writerId, "wrote to file");
            printf("[Writer %d] Wrote data to file\n", writerId);
        }
        else {
            log_message("Writer", writerId, "failed to write to file");
            printf("[Writer %d] Error writing to file\n", writerId);
        }

        fclose(file);
    }
    else {
        log_message("Writer", writerId, "failed to open file for writing");
        printf("[Writer %d] Error opening file for writing\n", writerId);
    }
    simulate_delay(100, 200);
}

DWORD WINAPI reader(LPVOID param) {
    int id = *(int*)param;

    while (1) {
        simulate_delay(500, DELAY_TIME);

        DWORD startWait = GetTickCount64();

        WaitForSingleObject(readerCountMutex, INFINITE);
        if (readerCount == 0) {
            WaitForSingleObject(resourceMutex, INFINITE); 
        }
        readerCount++;
        ReleaseMutex(readerCountMutex);

        DWORD endWait = GetTickCount64();
        totalReaderWaitTime += (endWait - startWait);
        readerOps++;

        log_message("Reader", id, "started reading");
        read_file(id);
        simulate_delay(500, RESOURCE_USE_TIME);
        log_message("Reader", id, "finished reading");

        WaitForSingleObject(readerCountMutex, INFINITE);
        readerCount--;
        if (readerCount == 0) {
            ReleaseMutex(resourceMutex); 
        }
        ReleaseMutex(readerCountMutex);
    }
    return 0;
}

DWORD WINAPI writer(LPVOID param) {
    int id = *(int*)param;

    while (1) {
        simulate_delay(500, DELAY_TIME);

        DWORD startWait = GetTickCount64();

        WaitForSingleObject(resourceMutex, INFINITE);

        DWORD endWait = GetTickCount64();
        totalWriterWaitTime += (endWait - startWait);
        writerOps++;

        log_message("Writer", id, "started writing");
        write_file(id);
        simulate_delay(500, RESOURCE_USE_TIME);
        log_message("Writer", id, "finished writing");

        ReleaseMutex(resourceMutex);
    }
    return 0;
}

int main() {
    srand((unsigned int)time(NULL));

    InitializeCriticalSection(&logLock);
    resourceMutex = CreateMutex(NULL, FALSE, NULL);
    readerCountMutex = CreateMutex(NULL, FALSE, NULL);

    HANDLE threads[MAX_READERS + MAX_WRITERS];
    int ids[MAX_READERS + MAX_WRITERS];

    for (int i = 0; i < MAX_READERS; i++) {
        ids[i] = i + 1;
        threads[i] = CreateThread(NULL, 0, reader, &ids[i], 0, NULL);
    }

    for (int i = 0; i < MAX_WRITERS; i++) {
        ids[MAX_READERS + i] = i + 1;
        threads[MAX_READERS + i] = CreateThread(NULL, 0, writer, &ids[MAX_READERS + i], 0, NULL);
    }

    Sleep(10000);

    for (int i = 0; i < MAX_READERS + MAX_WRITERS; i++) {
        TerminateThread(threads[i], 0);
        CloseHandle(threads[i]);
    }

    printf("\n--- Statistics ---\n");
    printf("Total Reader Wait Time: %.2f ms\n", totalReaderWaitTime);
    printf("Average Reader Wait Time: %.2f ms\n", readerOps > 0 ? totalReaderWaitTime / readerOps : 0.0);
    printf("Total Writer Wait Time: %.2f ms\n", totalWriterWaitTime);
    printf("Average Writer Wait Time: %.2f ms\n", writerOps > 0 ? totalWriterWaitTime / writerOps : 0.0);

    DeleteCriticalSection(&logLock);
    CloseHandle(resourceMutex);
    CloseHandle(readerCountMutex);

    return 0;
}


