#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFSIZE 524

int main(int argc, char **argv)
{
    int sockfd;
    int port;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    int message_size;
    int addr_len;char buffer[BUFSIZE];

    int filenum = 0;
    char* filename;

    FILE *fptr;

    if (argc != 2)
    {
        fprintf(stderr, "ERROR:Please provide a valid port\n");
        exit(1);
    }
    port = atoi(argv[1]);


    //create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR:creating socket\n");
        exit(1);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("ERROR:binding socket\n");
        exit(1);
    }

    addr_len = sizeof(clientaddr);

    //server loop
    while(1)
    {
        message_size = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addr_len);
        sprintf(filename, "%d", filenum);
        fptr = fopen(filename, "w");
        if (fptr == NULL)
        {
            perror("ERROR:opening file\n");
            exit(1);
        }
        fprintf(fptr, "%s", buffer);
        fclose(fptr);
    }

}
