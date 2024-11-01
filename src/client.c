#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define MSG_SIZE 256
#define SOCKET_DIR "/tmp/evinServerSocket"

int writeToServer(int *clientfd);

int main(int argc, char *argv[])
{
    int                clientfd;
    char               incomingMsg[MSG_SIZE];
    ssize_t            bytesRead;
    struct sockaddr_un serverAddress;
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
    clientfd = socket(AF_UNIX, SOCK_STREAM, 0); // NOLINT(android-cloexec-socket)
    if(clientfd < 0)
    {
        perror("Could not create socket");
        goto cleanup;
    }

    // defining the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, SOCKET_DIR, sizeof(serverAddress.sun_path) - 1);

    // connect to server
    if(connect(clientfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)))
    {
        perror("Could not connect to the server");
        goto cleanup;
    }

    // construct the data
    required_size = (size_t)snprintf(NULL, 0, "%s\n%s", filter, message) + 1;
    snprintf(msgBuffer, required_size, "%s\n%s", filter, message);

    // send the data
    if(write(clientfd, msgBuffer, MSG_SIZE) < 0)
    {
        perror("Could not send message to server");
        goto cleanup;
    }

    // recieve
    bytesRead = read(clientfd, incomingMsg, MSG_SIZE);
    if(bytesRead < 0)
    {
        perror("Could not read incoming data");
        goto cleanup;
    }
    printf("Recieved: %s\n", incomingMsg);

cleanup:
    close(clientfd);
    return EXIT_FAILURE;
}
