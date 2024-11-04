#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MSG_SIZE 256
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8000

int main(int argc, char *argv[])
{
    int                clientfd;
    char               incomingMsg[MSG_SIZE];
    ssize_t            bytesRead;
    struct sockaddr_in serverAddress;
    int                opt;
    const char        *filter;
    const char        *message;
    char               msgBuffer[MSG_SIZE];
    size_t             required_size;

    filter  = NULL;
    message = NULL;
    while((opt = getopt(argc, argv, "f:m:")) != -1)
    {
        switch(opt)
        {
            case 'f':
                filter = optarg;
                if(strcmp(filter, "upper") != 0 && strcmp(filter, "lower") != 0 && strcmp(filter, "none") != 0)
                {
                    fprintf(stderr, "Error: Invalid filter\n");
                    fprintf(stderr, "Usage for -f:\n  -upper\n  -lower\n  -none\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'm':
                message = optarg;
                break;
            default:
                fprintf(stderr, "Usage: -f filter -m message\n");
                return EXIT_FAILURE;
        }
    }

    if(filter == NULL || message == NULL)
    {
        fprintf(stderr, "Error: filter or message cannot be null\n");
        fprintf(stderr, "Usage: -f filter -m message\n");
        return EXIT_FAILURE;
    }

    // create socket
    clientfd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    if(clientfd < 0)
    {
        perror("Could not create socket");
        goto cleanup;
    }

    // defining the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_port        = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    // connect to server
    if(connect(clientfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Could not connect to the server");
        goto cleanup;
    }

    // construct the data
    required_size = (size_t)snprintf(NULL, 0, "%s\n%s", filter, message) + 1;
    snprintf(msgBuffer, required_size, "%s\n%s", filter, message);

    // send the data
    if(write(clientfd, msgBuffer, required_size) < 0)
    {
        perror("Could not send message to server");
        goto cleanup;
    }

    // receive
    bytesRead = read(clientfd, incomingMsg, MSG_SIZE);
    if(bytesRead < 0)
    {
        perror("Could not read incoming data");
        goto cleanup;
    }
    incomingMsg[bytesRead] = '\0';
    printf("Received: %s\n", incomingMsg);

cleanup:
    close(clientfd);
    return EXIT_FAILURE;
}
