#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>  

#define MAX_BUFFER 1  

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER + 1] = {0};  

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    fd_set read_fds;
    int max_sd;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_sd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buffer, MAX_BUFFER + 1, stdin);  
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, "q") == 0) {
                send(sock, buffer, strlen(buffer), 0);
                break;
            }

            send(sock, buffer, strlen(buffer), 0);
        }

        if (FD_ISSET(sock, &read_fds)) {
            int valread = read(sock, buffer, MAX_BUFFER);  
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("\033[H\033[J");  
                printf("%s\n", buffer);
            } else if (valread == 0) {
                printf("Server closed connection\n");
                break;
            } else {
                perror("recv");
                break;
            }
        }
    }

    close(sock);
    return 0;
}

