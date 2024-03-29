#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024


#define convert_ENDIANNES(ui)               (((ui & 0xFF) << 8) | (ui >> 8))

struct control_server_read_packet
{
    uint16_t op_code;
    uint16_t object;
    uint16_t property;
}
__attribute__( ( packed ) );
typedef struct control_server_read_packet control_server_read_packet_t;


struct control_server_write_packet
{
    uint16_t op_code;
    uint16_t object;
    uint16_t property;
    uint16_t value;
}
__attribute__( ( packed ) );
typedef struct control_server_write_packet control_server_write_packet_t;

void print_json(double timestamp, char *out1, char *out2, char *out3)
{
    printf("{\"timestamp\": %.0f, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
           timestamp, out1, out2, out3);
}

void control_channel_worker(int cntrl_sockfd, double op3, const struct sockaddr * pserv_addr)
{

    control_server_write_packet_t write_freq, write_amplitude;

    write_freq.op_code = 0x0200U;
    write_freq.object = 0x0100U;
    write_freq.property = convert_ENDIANNES(255);

    write_amplitude.op_code = 0x0200U;
    write_amplitude.object = 0x0100U;
    write_amplitude.property = convert_ENDIANNES(170);
    
    if(op3 >= 3.0)
    {
        write_freq.value = convert_ENDIANNES(1000);
        write_amplitude.value = convert_ENDIANNES(8000);
    }
    else
    {
        write_freq.value = convert_ENDIANNES(2000);
        write_amplitude.value = convert_ENDIANNES(4000);
    }

    sendto(cntrl_sockfd, (const void *)&write_freq, sizeof(write_freq),
        MSG_CONFIRM, pserv_addr,  
            sizeof(struct sockaddr_in)); 

    sendto(cntrl_sockfd, (const void *)&write_amplitude, sizeof(write_amplitude),
        MSG_CONFIRM, pserv_addr,  
            sizeof(struct sockaddr_in)); 
}

int main()
{
    struct sockaddr_in serv_addr, cntrl_serv_addr;
    int cntrl_sockfd;
    int sockfd[3] = {0}; // Socket file descriptors for each port
    char buffer[BUFFER_SIZE] = {0};
    double timestamp = 0;
    char out1[BUFFER_SIZE] = "--"; // Initialize output strings to "--"
    char out2[BUFFER_SIZE] = "--";
    char out3[BUFFER_SIZE] = "--";

    /* Create a UDP socket for control channel */
    if ((cntrl_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation error");
        return 1;
    }

    /* Setup control channel server address */
    memset(&cntrl_serv_addr, '0', sizeof(serv_addr));
    cntrl_serv_addr.sin_family = AF_INET;
    cntrl_serv_addr.sin_port = htons(4000);
    if (inet_pton(AF_INET, "127.0.0.1", &cntrl_serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        return 1;
    }

    // Connect to each TCP port
    for (int i = 0; i < 3; ++i)
    {
        if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation error");
            return 1;
        }
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(4001 + i);
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        {
            perror("Invalid address/ Address not supported");
            return 1;
        }
        if (connect(sockfd[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            perror("Connection Failed");
            return 1;
        }
    }

    while (1)
    {
        // Reset output strings for each iteration
        strcpy(out1, "--");
        strcpy(out2, "--");
        strcpy(out3, "--");

        // Receive data from each socket
        // Receive data from each socket
        for (int i = 0; i < 3; ++i)
        {
            int bytes_received = recv(sockfd[i], buffer, BUFFER_SIZE, MSG_DONTWAIT);
            if (bytes_received > 0)
            {
                // Ensure buffer is null-terminated
                buffer[bytes_received] = '\0';
                // Print received data, its length, and socket index
                // printf("Received data from socket %d (length %d): %s", i, bytes_received, buffer);

                // Tokenize buffer based on newline character
                char *token = strtok(buffer, "\n");
                char *last_token = NULL;
                while (token != NULL)
                {
                    last_token = token;
                    token = strtok(NULL, "\n");
                }

                if (last_token != NULL)
                {
                    switch (i)
                    {
                    case 0:
                        strncpy(out1, last_token, BUFFER_SIZE);
                        break;
                    case 1:
                        strncpy(out2, last_token, BUFFER_SIZE);
                        break;
                    case 2:
                        strncpy(out3, last_token, BUFFER_SIZE);
                        control_channel_worker(cntrl_sockfd, atof(last_token), (const struct sockaddr *) &cntrl_serv_addr);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        // Get current time in milliseconds since Unix epoch
        struct timeval tv;
        gettimeofday(&tv, NULL);
        timestamp = (double)(tv.tv_sec) * 1000 + (double)(tv.tv_usec) / 1000;

        // Print JSON object with the most recent values
        print_json(timestamp, out1, out2, out3);

        // Sleep for 100 milliseconds
        usleep(20000);
    }

    // Close sockets
    for (int i = 0; i < 3; ++i)
    {
        close(sockfd[i]);
    }

    return 0;
}

