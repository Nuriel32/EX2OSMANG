#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define SRV_FILE "srv_to"
#define ERROR_EXIT(msg) { perror(msg); exit(EXIT_FAILURE); }

volatile int response_received = 0; // דגל למעקב אחרי תשובת השרת

// טיפול באות SIGUSR2 מהשרת
void sigusr2_handler(int signum) {
    response_received = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("[CLIENT] Usage: %s <server_pid> <num1> <op> <num2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t server_pid = atoi(argv[1]);
    int num1 = atoi(argv[2]);
    int op = atoi(argv[3]);
    int num2 = atoi(argv[4]);

    char buffer[256];
    int fd, retries = 10;

    // רישום מנהל אות ל-SIGUSR2
    signal(SIGUSR2, sigusr2_handler);

    // ניסיון לכתוב לקובץ "srv_to"
    for (int i = 0; i < retries; i++) {
        fd = open(SRV_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd >= 0) {
            // כתיבה ל-srv_to
            sprintf(buffer, "%d\n%d\n%d\n%d\n", getpid(), num1, op, num2);
            write(fd, buffer, strlen(buffer));
            close(fd);

            printf("[CLIENT] Wrote to srv_to and notified server (PID=%d).\n", server_pid);
            kill(server_pid, SIGUSR1); // שליחת אות לשרת
            break;
        } else {
            printf("[CLIENT] srv_to unavailable. Retrying in 1-5 seconds...\n");
            sleep(rand() % 5 + 1); // המתנה אקראית
        }

        if (i == retries - 1) {
            printf("[CLIENT] Failed to access srv_to after 10 attempts. Exiting.\n");
            exit(EXIT_FAILURE);
        }
    }

    // המתנה לתשובת השרת
    char client_file[256];
    sprintf(client_file, "%d_client_to", getpid());

    while (!response_received) {
        pause();
    }

    // קריאה מתשובת השרת
    fd = open(client_file, O_RDONLY);
    if (fd < 0) {
        perror("[CLIENT] Error opening client response file");
        exit(EXIT_FAILURE);
    }

    read(fd, buffer, sizeof(buffer));
    close(fd);
    printf("[CLIENT] Server response: %s", buffer);

    // מחיקת קובץ הלקוח
    unlink(client_file);
    return 0;
}
