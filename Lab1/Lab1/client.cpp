#include <Windows.h>
#include <stdio.h>
#define BUFFER_SIZE 1024
#define PIPE_NAME L"\\\\.\\pipe\\LogPipe"

int main() {
    HANDLE pipe;
    char buffer[BUFFER_SIZE];
    DWORD bytesWritten;

    while (1) {
        pipe = CreateFile(
            PIPE_NAME,
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (pipe != INVALID_HANDLE_VALUE) {
            break;
        }

        Sleep(1000);
    }

    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        if (!WriteFile(pipe, buffer, strlen(buffer) + 1, &bytesWritten, NULL)) {
            fprintf(stderr, "Failed to write to pipe. Error: %ld\n", GetLastError());
            break;
        }
    }

    CloseHandle(pipe);
    return 0;
}
