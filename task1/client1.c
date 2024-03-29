#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024

void print_json(double timestamp, char *out1, char *out2, char *out3) {
    printf("{\"timestamp\": %.0f, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
           timestamp, out1, out2, out3);
}

int main() {
    struct sockaddr_in serv_addr;
    int sockfd[3] = {0}; // Socket file descriptors for each port
    char buffer[BUFFER_SIZE] = {0};
    double timestamp = 0;
    char out1[BUFFER_SIZE] = "--"; // Initialize output strings to "--"
    char out2[BUFFER_SIZE] = "--";
    char out3[BUFFER_SIZE] = "--";

    // Connect to each TCP port
    for (int i = 0; i < 3; ++i) {
        if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            return 1;
        }
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(4001 + i);
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            return 1;
        }
        if (connect(sockfd[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection Failed");
            return 1;
        }
    }

    while (1) {
        // Reset output strings for each iteration
        strcpy(out1, "--");
        strcpy(out2, "--");
        strcpy(out3, "--");

        // Receive data from each socket
       // Receive data from each socket
for (int i = 0; i < 3; ++i) {
    int bytes_received = recv(sockfd[i], buffer, BUFFER_SIZE, MSG_DONTWAIT);
    if (bytes_received > 0) {
        // Ensure buffer is null-terminated
        buffer[bytes_received] = '\0';
   // Print received data, its length, and socket index
     //printf("Received data from socket %d (length %d): %s", i, bytes_received, buffer);

        // Tokenize buffer based on newline character
        char *token = strtok(buffer, "\n");
        char *last_token = NULL;
        while (token != NULL) {
            last_token = token;
            token = strtok(NULL, "\n");
        }

        if (last_token != NULL) {
        // Get current time in milliseconds since Unix epoch
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timestamp = (double)(tv.tv_sec) * 1000 + (double)(tv.tv_usec) / 1000;
            switch (i) {
                case 0:
                    strncpy(out1, last_token, BUFFER_SIZE);
                    break;
                case 1:
                    strncpy(out2, last_token, BUFFER_SIZE);
                    break;
                case 2:
                    strncpy(out3, last_token, BUFFER_SIZE);
                    break;
                default:
                    break;
            }
        }
    }
}

        // Print JSON object with the most recent values
        print_json(timestamp, out1, out2, out3);

        // Sleep for 100 milliseconds
        usleep(100000);
    }

    // Close sockets
    for (int i = 0; i < 3; ++i) {
        close(sockfd[i]);
    }

    return 0;
}
