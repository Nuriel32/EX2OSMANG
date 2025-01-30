/***Moria gavra
305151938
keren or gilad 
315127548 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <ctype.h>



#define SRV_FILE "srv_to"
#define ERROR_EXIT(msg) { perror(msg); exit(EXIT_FAILURE); }


int is_integer(const char *str) {
    if (*str == '\0') return 0; 
    if (*str == '-' || *str == '+') str++; 
    while (*str) {
        if (!isdigit(*str)) return 0; 
        str++;
    }
    return 1;
}

void validate_arguments(int argc, char *argv[]) {
    if (argc != 5) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }

    if (!is_integer(argv[1])) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }

    if (!is_integer(argv[2])) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }

    if (!is_integer(argv[3])) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }
    int op = atoi(argv[3]);
    if (op < 1 || op > 4) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }

    if (!is_integer(argv[4])) {
        printf("ERROR_FROM_EX2\n");
        exit(EXIT_FAILURE);
    }
}


volatile int response_received = 0; 

void sigusr2_handler(int signum) {
    response_received = 1;
}

void alarm_handler(int signum) {
    printf("Client closed beacuse no response was received from the server for 30 seconds");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    validate_arguments(argc, argv);
    srand(time(NULL));
    int random_sleep_time = rand() % 10 + 15; //for printing
    sleep(random_sleep_time); // for printing.



    pid_t server_pid = atoi(argv[1]);
    int num1 = atoi(argv[2]);
    int op = atoi(argv[3]);
    int num2 = atoi(argv[4]);

    char buffer[256];
    int fd, retries = 10;

    signal(SIGUSR2, sigusr2_handler);
    signal(SIGALRM,  alarm_handler);


    int clientpid=getpid();

    for (int i = 0; i < retries; i++) {
        //We use O_EXCL HERE IN ORDER TO MAKE SURE THAT That the file is not exist which means only one writer in the producer consumer architure which the client is the consumer. 
        //and this is how we avoid two clients override each other. 
        //the number 0644 represent permision mode for a file.
        fd = open(SRV_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd >= 0) {
            sprintf(buffer, "%d\n%d\n%d\n%d\n", clientpid, num1, op, num2);
            write(fd, buffer, strlen(buffer));
            close(fd);
        
            printf("[CLIENT] Wrote to srv_to and notified server (PID=%d).\n", server_pid);
            kill(server_pid, SIGUSR1); // we send here a signal to the server in order to notice him we work. 
            printf("end of stage D\n");
            alarm(30);
            break;
        } else {
            printf("[CLIENT] srv_to unavailable. Retrying in 1-5 seconds...\n");
            sleep((rand() % 6)); 
        }

        if (i == retries - 1) {
            printf("[CLIENT] Failed to access srv_to after 10 attempts. Exiting.\n");
            exit(EXIT_FAILURE);
        }
        
    }
     printf("end of stage E\n");


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
    printf("end of stage J\n");


    unlink(client_file);
    printf("end of stage D\n");

    return 0;
}