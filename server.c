#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SRV_FILE "srv_to"
#define ERROR_EXIT(msg) { perror(msg); exit(EXIT_FAILURE); }

// טיפול בתהליכי זומבי
void sigchld_handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    printf("[SERVER] Cleaned up terminated child processes.\n");
}

// טיפול בבקשת לקוח
void sigusr1_handler(int signum) {
    int fd;
    char buffer[256];
    char client_file[256];
    pid_t client_pid;
    int num1, num2, op, result;

    printf("[SERVER] Received signal (SIGUSR1). Processing client request...\n");

    // פתיחת הקובץ המשותף לקריאה
    fd = open(SRV_FILE, O_RDONLY);
    if (fd < 0) {
        perror("[SERVER] Error opening srv_to");
        return;
    }

    // קריאה מתוכן הקובץ
    read(fd, buffer, sizeof(buffer));
    close(fd);

    // ניתוח הבקשה מהלקוח
    sscanf(buffer, "%d\n%d\n%d\n%d", &client_pid, &num1, &op, &num2);
    printf("[SERVER] Parsed request: PID=%d, num1=%d, op=%d, num2=%d\n", client_pid, num1, op, num2);

    // מחיקת הקובץ "srv_to" מיד לאחר הקריאה
    if (unlink(SRV_FILE) != 0) {
        perror("[SERVER] Error deleting srv_to");
        return;
    }

    // יצירת תהליך ילד לטיפול בבקשה
    pid_t pid = fork();
    if (pid < 0) {
        perror("[SERVER] Fork failed");
        return;
    }

    if (pid == 0) { // תהליך ילד
        // יצירת שם הקובץ הייחודי עבור הלקוח
        sprintf(client_file, "%d_client_to", client_pid);
        fd = open(client_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("[SERVER-CHILD] Error creating client file");
            exit(EXIT_FAILURE);
        }

        // ביצוע החישוב
        switch (op) {
            case 1: result = num1 + num2; break;
            case 2: result = num1 - num2; break;
            case 3: result = num1 * num2; break;
            case 4: result = (num2 != 0) ? (num1 / num2) : 0; break;
            default: result = 0;
        }

        // כתיבת התוצאה לקובץ של הלקוח
        sprintf(buffer, "Result: %d\n", result);
        write(fd, buffer, strlen(buffer));
        close(fd);

        // שליחת אות ללקוח לסיום הבקשה
        kill(client_pid, SIGUSR2);
        printf("[SERVER-CHILD] Completed request for PID=%d.\n", client_pid);
        exit(0);
    }
}

// פונקציית main של השרת
int main() {
    // מחיקת הקובץ "srv_to" אם קיים
    unlink(SRV_FILE);

    // רישום מנהלי אותות
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGCHLD, sigchld_handler);

    printf("[SERVER] Server running with PID: %d\n", getpid());

    // לולאה להמתנה לאותות
    while (1) {
        pause();
    }

    return 0;
}
