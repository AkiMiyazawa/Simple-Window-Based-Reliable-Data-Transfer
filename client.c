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
#define MAXFILE 104857600

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
	char buffer[MAXFILE] = {0};
	struct sockaddr_in serveraddr;

	char *hostname;
	char *filename;
	int port;
	struct hostent *he;

	FILE *fptr;

	struct packet *p1;
	p1->seq_num = rand() % 25600;
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
	server = gethostbyname(hostname);
	if (server == NULL) {
		printf("ERROR: Host is not found.\n");
		exit(1);
	}
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
	fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        printf("ERROR: Cannot open file \n");
        exit(1);
    }

	

	if(sendto(sockfd, p1, sizeof(p1), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("ERROR:sending message on socket failed.\n");
		exit(1);
	} 

	fgets(buffer, MAXFILE, (FILE*)fptr);



	while (!feof(fp)) {
char* buffer = new char[100];
size_t count = fread(buffer,sizeof(char),100,fp);

//if count < 100, then the end of file was reached, for sure.
process(buffer);
delete buffer;

	int is_file_sent = 0;
	while(1){
		struct packet response_packet;
		struct packet new_packet;
		if (recvfrom(sockfd, &response_packet, sizeof(response_packet), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) >= 1){
			if(is_file_sent){
				new_packet.fin = 1;
				new_packet.seq_num = response_packet.ack;
			}
			if (!is_file_sent){
				char buffer[PAYLOAD] = {0};
				size_t count = fread(&buffer,sizeof(char),PAYLOAD,fptr);
				if (count < PAYLOAD){
					is_file_sent = 1;
				}
			}
			if (response_packet.syn == 1 && response_packet.ack > 0){
				// the third part of handshake
				new_packet.syn = 0;
				new_packet.ack = (response_packet.seq_num + 1) % 25600;
				new_packet.seq_num = response_packet.ack;
			}
			else if (response_packet.fin == 1){
				//finish
			}
			else if(response_packet.ack > 0){

			}
		}

	}


}
