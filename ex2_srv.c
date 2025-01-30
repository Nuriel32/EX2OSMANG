/***Moria gavra
305151938
keren or gilad 
315127548 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SRV_FILE "srv_to"
#define ERROR_EXIT(msg) { perror(msg); exit(EXIT_FAILURE); }

void server_activity() {
    alarm(60); // Reset the alarm to 60 seconds
    printf("[SERVER] Activity detected. Alarm reset.\n");
}

void sigchld_handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    printf("[SERVER] Cleaned up terminated child processes.\n");
}

void handle_alarm() {
    printf("The server was closed because no service request was received for the last 60 seconds.\n");
    exit(0);
}

void process_client_request() {
    int fd;
    char buffer[256];
    char client_file[256];
    pid_t client_pid;
    int num1, num2, result, op;

    printf("[SERVER-CHILD] Processing client request...\n");

    fd = open(SRV_FILE, O_RDONLY);
    if (fd < 0) {
        perror("[SERVER-CHILD] Error opening srv_to");
        exit(EXIT_FAILURE);
    }

    read(fd, buffer, sizeof(buffer));
    close(fd);

    sscanf(buffer, "%d\n%d\n%d\n%d", &client_pid, &num1, &op, &num2);
    printf("[SERVER-CHILD] Parsed request: PID=%d, num1=%d, op=%d, num2=%d\n", client_pid, num1, op, num2);

    if (unlink(SRV_FILE) != 0) {
        perror("[SERVER-CHILD] Error deleting srv_to");
        exit(EXIT_FAILURE);
    }
    printf("end of stage D\n");


    sprintf(client_file, "%d_client_to", client_pid);
    fd = open(client_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("end of stage H\n");

    if (fd < 0) {
        perror("[SERVER-CHILD] Error creating client file");
        exit(EXIT_FAILURE);
    }
    if(op==4 && num2==0){
        write(fd, "Error: Division by zero\n", 24);
        close(fd);
        kill(client_pid, SIGUSR2);
        printf("[SERVER-CHILD] Completed request for PID=%d.\n", client_pid);
        exit(0);
    }
    switch (op) {
        case 1: result = num1 + num2; break;
        case 2: result = num1 - num2; break;
        case 3: result = num1 * num2; break;
        case 4: result = (num2 != 0) ? (num1 / num2) : 0; break;
        default: result = 0; // Invalid operation
    }
    
    sprintf(buffer, "Result: %d\n", result);
    write(fd, buffer, strlen(buffer));
    close(fd);

    kill(client_pid, SIGUSR2);
    printf("[SERVER-CHILD] Completed request for PID=%d.\n", client_pid);
    printf("end of stage I\n");


    exit(0); // Terminate the child process
}

void sigusr1_handler(int signum) {
    server_activity(); // Reset the alarm upon receiving activity

    pid_t pid = fork();
    if (pid < 0) {
        perror("[SERVER] Fork failed");
        return;
    }

    if (pid == 0) {
        printf("end of stage F\n");
        process_client_request();
    }
}

int main() {
    unlink(SRV_FILE);
    printf("end of stage C\n");
    server_activity();

    signal(SIGUSR1, sigusr1_handler); // Handle client requests
    signal(SIGCHLD, sigchld_handler); // Handle child process cleanup
    signal(SIGALRM, handle_alarm);    // Handle inactivity timeout

    printf("[SERVER] Server running with PID: %d\n", getpid());

    // Main loop to wait for signals
    while (1) {
        pause(); // Wait for a signal
    }
    while(wait(NULL)!=-1);
    return 0;
}

