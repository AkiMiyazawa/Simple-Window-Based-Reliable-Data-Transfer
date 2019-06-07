#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdint.h>
#include <netdb.h>
#include <inttypes.h>

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

	char data[PAYLOAD];
	//char* data;
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

	int addr_len;
	int message_size;

	FILE *fptr;


	struct packet ps;
	struct packet pr;

	int cwnd = 0;
	int ssthresh = 0;

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

	he = gethostbyname(hostname);

	if(he == NULL)
	{
		perror("ERROR:Could not get host\n");
		exit(1);
	}

	serveraddr.sin_family = AF_INET;
	bcopy((char*)he->h_addr, (char*)&serveraddr.sin_addr.s_addr, he->h_length);
	serveraddr.sin_port = htons(port);
	

	addr_len = sizeof(serveraddr);

	//construct syn packet
	memset((char *) &ps, 0, sizeof(ps));
	memset((char *) &pr, 0, sizeof(pr));

	srand(time(0));

	ps.seq_num = rand() % 2560 + 1;
	ps.ack_num = 0;
	ps.ack = 0;
	ps.syn = 1;
	ps.fin = 0;
	//ps.data = buffer;

	if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("ERROR:sending message");
		exit(1);
	} 

	fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);

	message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);

	fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

	fprintf(stdout, "%d", sizeof(ps));

	fptr = fopen(filename, "r");
	if (fptr == NULL)
	{
		perror("ERROR:opening file\n");
		exit(1);
	}

	fgets(ps.data, PAYLOAD, (FILE*)fptr);
	fclose(fptr);


	

	//if(sendto(sockfd, p1, sizeof(p1), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	//{
	//	perror("ERROR:sending message");
	//	exit(1);
	//} 

}
