#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define MAX_CLIENTS 5
#define PORT 8000

void recieve_message(int clientfd);
int  apply_filter(const char *filter, char *msg, size_t msg_size);
void cleanup(int sig) __attribute__((noreturn));

int main(void)
{
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    int                serverfd;
    int                clientfd;
    socklen_t          addr_len = sizeof(clientAddress);
    pid_t              pid;

    // Handle CTRL-C
    signal(SIGINT, cleanup);

    // Create a TCP socket
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd < 0)
    {
        perror("Could not open socket");
        return EXIT_FAILURE;
    }

    // Set up the server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons(PORT);

    // Bind the socket to the specified port
    if(bind(serverfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Failed to bind server");
        goto cleanup;
    }

    // Start listening for incoming connections
    printf("Started listening on port %d...\n", PORT);
    if(listen(serverfd, MAX_CLIENTS) < 0)
    {
        perror("Server could not listen");
        goto cleanup;
    }

    // Accept clients in a loop
    while(1)
    {
        clientfd = accept(serverfd, (struct sockaddr *)&clientAddress, &addr_len);
        if(clientfd < 0)
        {
            perror("Could not accept client request");
        }

        pid = fork();
        if(pid < 0)
        {
            perror("Failed to fork");
            close(clientfd);
        }
        else if(pid == 0)
        {
            close(serverfd);
            recieve_message(clientfd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(clientfd);
        }
    }

    // Cleanup on error
cleanup:
    close(serverfd);
    return EXIT_FAILURE;
}

void cleanup(int sig)
{
    (void)sig;
    _exit(EXIT_SUCCESS);
}

int apply_filter(const char *filter, char *msg, size_t msg_size)
{
    if(filter == NULL)
    {
        fprintf(stderr, "Error: Filter is NULL\n");
        return -1;
    }

    // Upper case
    if(strcmp(filter, "upper") == 0)
    {
        for(size_t i = 0; i < msg_size && msg[i] != '\0'; i++)
        {
            msg[i] = (char)toupper((unsigned char)msg[i]);
        }
        return 0;
    }

    // Lower case
    if(strcmp(filter, "lower") == 0)
    {
        for(size_t i = 0; i < msg_size && msg[i] != '\0'; i++)
        {
            msg[i] = (char)tolower((unsigned char)msg[i]);
        }
        return 0;
    }

    // None
    if(strcmp(filter, "none") == 0)
    {
        return 0;
    }

    // Error case
    return -1;
}

void recieve_message(int clientfd)
{
    char buff[BUFFER_SIZE];

    const char *filter;
    char       *message;
    char       *state;
    ssize_t     bytesRead = read(clientfd, buff, BUFFER_SIZE - 1);
    if(bytesRead == -1)
    {
        fprintf(stderr, "Error: couldn't read from client\n");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    printf("Received data...\n");

    // Null terminate the end of the buffer
    buff[bytesRead] = '\0';

    // Split the argument and the message
    filter  = strtok_r(buff, "\n", &state);
    message = strtok_r(NULL, "\n", &state);

    // Compare the strings and apply the filter
    if(apply_filter(filter, message, BUFFER_SIZE) == -1)
    {
        fprintf(stderr, "Error: Filter is invalid\n");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    printf("Sending requested data back...\n");
    // Write back to the client
    if(write(clientfd, message, BUFFER_SIZE - 1) < 0)
    {
        fprintf(stderr, "Error trying to write to client\n");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    close(clientfd);
}
