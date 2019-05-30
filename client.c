#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PAYLOAD 512
#define MAXSEQ 25600

struct packet
{
	int16_t seq_num;
	int16_t ack_num;
	int16_t ack;
	int16_t syn;
	int16_t fin;
	int16_t filler;

	//char data[PAYLOAD];
	char* data;
};

int main(int argc, char **argv)
{
	int sockfd;
	char buffer[PAYLOAD] = {0};
	struct sockaddr_in serveraddr;

	char *hostname;
	char *filename;
	int port;
	struct hostent *he;

	FILE *fptr;

	struct packet *p1;
	p1->seq_num = 0;
	p1->ack_num = 0;
	p1->ack = 0;
	p1->syn = 1;
	p1->fin = 0;
	//buffer = {0};
	p1->data = buffer;

	if(argc != 4)
	{
		perror("ERROR:Please provide hostname port filename\n");
		exit(1);
	}

	//hostname = argv[1];
	/*if((he = gethostbyname(argv[1])) == NULL)
	{
		perror("ERROR:hostname\n");
		exit(1);
	}*/
	hostname = argv[1];
	port = atoi(argv[2]);
	filename = argv[3];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("ERROR:opening socket\n");
		exit(1);
	}

	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	he = gethostbyname(hostname);
	//serveraddr.sin_addr = *((struct in_addr *)he->h_addr);

	/*
	fptr = fopen(filename, "r");
	if (fptr == NULL)
	{
		perror("ERROR:opening file\n");
		exit(1);
	}

	fgets(buffer, PAYLOAD, (FILE*)fptr);
	fclose(fptr);
	*/

	

	if(sendto(sockfd, p1, sizeof(p1), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("ERROR:sending message");
		exit(1);
	} 

}