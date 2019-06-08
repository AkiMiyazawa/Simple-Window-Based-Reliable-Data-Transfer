#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <inttypes.h>

#define BUFSIZE 524
#define PAYLOAD 512

struct packet
{
	int16_t seq_num;
	int16_t ack_num;
	int8_t ack;
	int8_t syn;
	int8_t fin;
	int16_t size;
	int8_t dup;

	char data[PAYLOAD];
	//char* data;
};



int filenum;
FILE *fptr;
char* filename;

void handle(int sig)
{
	fclose(fptr);
	fptr = fopen(filename, "w");
	fprintf(fptr, "%s", "C-c");
	fclose(fptr);
	exit(0);
}


int main(int argc, char **argv)
{
	signal(SIGTERM, handle); //shell command kill
	signal(SIGQUIT, handle); //C - \
	int sockfd;
	int port;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int message_size;
	int addr_len;
	char buffer[BUFSIZE];

	struct packet ps;
	struct packet pr;

	int cwnd = 0;
	int ssthresh = 0;

	filenum = 0;
	

	int16_t nextSeq;
	int16_t currAck;

	if (argc != 2)
	{
		fprintf(stderr, "ERROR:Please provide a valid port\n");
		exit(1);
	}
	port = atoi(argv[1]);


	//create socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

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

	
	while(1)
	{
		message_size = 0;

		memset((char *) &ps, 0, sizeof(ps));
		memset((char *) &pr, 0, sizeof(pr));
		while(message_size <= 0)
		{
			message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *) &clientaddr, &addr_len);
		}
		
		fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

		if(pr.syn == 0)
		{
			perror("ERROR:syn not set\n");
		}
		filenum += 1;

		int length = snprintf(NULL, 0, "%d.file", filenum);
		filename = malloc(length + 1);
		snprintf(filename, length + 1, "%d.file", filenum);

		//sprintf(filename, "%d", filenum);
		fptr = fopen(filename, "w");
		if (fptr == NULL)
		{
			perror("ERROR:opening file\n");
			exit(1);
		}


		//set seq num
		srand(time(0));
		ps.seq_num = rand() % 25601;

		//set ack num
		if(pr.seq_num == 25600)
			ps.ack_num = 0;
		else
			ps.ack_num = pr.seq_num + 1;


		//set flags
		ps.ack = 1;
		ps.syn = 1;
		ps.fin = 0;
		// ps.dup = 0;

		currAck = ps.ack_num;
		nextSeq = ps.seq_num + 1;
		if(nextSeq == 25601)
				nextSeq = 0;

		//send SYN response
		if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
		{
			perror("ERROR:sending message\n");
			exit(1);
		} 

		fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
		memset((char *) &pr, 0, sizeof(pr));

		struct timeval timeout = {10, 0};
		//tv.tv_sec = 0;
		//tv.tv_usec = 10000000;
		//tv.tv_usec = 100000;

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)) < 0)
		{
			perror("ERROR:setting timeout\n");
		}

		message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *) &clientaddr, &addr_len);

		fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
		
		while(pr.fin == 0 && message_size >= 0)
		{
			//make sure message acked last sent and seq num is expected
			if(pr.ack_num < nextSeq || pr.seq_num != currAck)
			{
				//need to retransmit
				if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
				{
					perror("ERROR:sending message\n");
					exit(1);
				} 	

				fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
				
			}
			else
			{
			
				fwrite(pr.data, 1, pr.size, fptr);

				//temp
				//fprintf(stdout, "WRITE %s\nFILE %s\n", pr.data, filename);

				memset((char *) &ps, 0, sizeof(ps));
				//send message acking current message
				ps.seq_num = nextSeq;
				currAck = (currAck + pr.size) % 25600;
				ps.ack_num = currAck;
				ps.ack = 1;
				ps.syn = 0;
				ps.fin = 0;
				// ps.dup = 0;

				nextSeq += 1;
				//nextSeq += sizeof(ps);
				if(nextSeq > 25600)
					nextSeq -= 25600;

				if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
				{
					perror("ERROR:sending message\n");
					exit(1);
				} 	

				fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
			}
		
			//get new message
			memset((char *) &pr, 0, sizeof(pr));
			message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *) &clientaddr, &addr_len);
			if(message_size < 0){
				perror("ERROR: message not received.\n");
				exit(1);
			}
			fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

		}
		//recieved packet with FIN or timeout

		if(message_size >= 0)
		{
			//FIN
			memset((char *) &ps, 0, sizeof(ps));
			ps.seq_num = nextSeq;
			ps.ack_num = (pr.seq_num + 1) % 25600;
			ps.ack = 1;
			ps.syn = 0;
			ps.fin = 0;
			// ps.dup = 0;

			//nextSeq += sizeof(ps);
			/*nextSeq += 1;
			if(nextSeq > 25600)
				nextSeq -= 25600;*/
			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
			{
				perror("ERROR:sending message\n");
				exit(1);
			} 	
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
			memset((char *) &ps, 0, sizeof(ps));
			ps.seq_num = nextSeq;
			ps.ack_num = 0;
			ps.ack = 0;
			ps.syn = 0;
			ps.fin = 1;
			// ps.dup = 0;

			nextSeq = (nextSeq + 1) % 25600;

			do
			{
				if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
				{
					perror("ERROR:sending message\n");
					exit(1);
				} 	
				fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);


				memset((char *) &pr, 0, sizeof(pr));
				message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *) &clientaddr, &addr_len);
				if(message_size < 0){
				perror("ERROR: message not received.\n");
				exit(1);
			}
				//if message_size <= 1
				fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

			}while(pr.ack_num != nextSeq);
		}
		
		
		fclose(fptr);
	}

}
