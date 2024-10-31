#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define BUFFER_SIZE 256
//  #define PERMISSION 0666
#define MAX_CLIENTS 5
#define SOCKET_DIR "/tmp/evinServerSocket"

void recieve_message(int clientfd);
int  apply_filter(const char *filter, char *msg, size_t msg_size);
void cleanup(int sig) __attribute__((noreturn));

int main(void)
{
    struct sockaddr_un serverAddress;
    struct sockaddr_un clientAddress;
    socklen_t          addressLength;
    int                serverfd;
    pid_t              pid;

    // Handle CTRL-C
    signal(SIGINT, cleanup);

    // unlink and existing then make a socket
    if(unlink(SOCKET_DIR) < 0)
    {
        printf("No directory found\n");
    }

    serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(serverfd < 0)
    {
        perror("Could not open socket");
        return EXIT_FAILURE;
    }

    // edit the values of the struct
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, SOCKET_DIR, sizeof(serverAddress.sun_path) - 1);

    //    // bind the fd
    if(bind(serverfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Failed to bind server");
        goto cleanup;
    }

    // listen
    printf("Started listening...");
    if(listen(serverfd, MAX_CLIENTS) < 0)
    {
        perror("Server could not listen");
        goto cleanup;
    }

    addressLength = sizeof(clientAddress);

    // accept clients in a loop
    while(1)
    {
        int clientfd = accept(serverfd, (struct sockaddr *)&clientAddress, &addressLength);
        if(clientfd < 0)
        {
            perror("Could not accept client request");
            goto cleanup;
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

    // cleanup on error
cleanup:
    close(serverfd);
    return EXIT_FAILURE;
}

void cleanup(int sig)
{
    (void)sig;
    //    unlink(SOCKET_DIR);
    _exit(EXIT_SUCCESS);
}

int apply_filter(const char *filter, char *msg, size_t msg_size)
{
    if(filter == NULL)
    {
        fprintf(stderr, "Error: Filter is NULL\n");
        return -1;
    }

    // upper
    if(strcmp(filter, "upper") == 0)
    {
        for(size_t i = 0; i < msg_size && msg[i] != '\0'; i++)
        {
            msg[i] = (char)toupper((unsigned char)msg[i]);
        }
        return 0;
    }

    // lower
    if(strcmp(filter, "lower") == 0)
    {
        for(size_t i = 0; i < msg_size && msg[i] != '\0'; i++)
        {
            msg[i] = (char)tolower((unsigned char)msg[i]);
        }
        return 0;
    }

    // none
    if(strcmp(filter, "none") == 0)
    {
        return 0;
    }

    // error case
    return -1;
}

void recieve_message(int clientfd)
{
    // int  serverfd;
    // int  clientfd = *((int *)arg);    // Cast and dereference the argument
    char buff[BUFFER_SIZE];

    const char *filter;
    char       *message;
    char       *state;
    ssize_t     bytesRead = read(clientfd, buff, BUFFER_SIZE - 1);
    if(bytesRead == -1)
    {
        fprintf(stderr, "Error: couldn't read from FIFO\n");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    printf("Recieved data...\n");

    // Null terminate the end of the buffer
    buff[bytesRead] = '\0';

    // Open the server fd
    // serverfd = open(SERVER_PATH, O_WRONLY | O_CLOEXEC);
    // if(serverfd == -1)
    // {
    //     fprintf(stderr, "Error opening server file descriptor\n");
    //     pthread_exit(NULL);
    // }

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
    // Write back to the other server
    if(write(clientfd, message, BUFFER_SIZE - 1) < 0)
    {
        fprintf(stderr, "Error trying to write to client\n");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    close(clientfd);
}
