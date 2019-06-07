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

	socklen_t addr_len;
	int message_size;

	int16_t currSeq; //current seq number
	int16_t currAck; //expected seq number

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

	ps.seq_num = rand() % 25600;
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

	currSeq = ps.seq_num + 1;
	if(currSeq == 25601)
		currSeq = 0;

	fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);

	message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);

	fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);


	fptr = fopen(filename, "r");

	if (fptr == NULL)
	{
		perror("ERROR:opening file\n");
		exit(1);
	}


	size_t buflen;
	int end_process = 0;
	int begin_process = 1;
	while(1){
		buflen = fread(buffer, PAYLOAD, 1, (FILE*)fptr);
		if (buflen < 1) {
	        if (!feof(fptr)) {
	            perror("ERROR:reading file\n");
				exit(1);
	        }
	        else{
	        	end_process = 1;
	        	continue;
	        }
        	break;
	    }
	    //3 way hand shake thing
		if(begin_process){
			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
				perror("ERROR:sending message");
				exit(1);
			} 
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);
			message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
			fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

			if(pr.syn && pr.ack){
				ps.seq_num = pr.ack_num;
				ps.ack_num = (pr.seq_num + 1) % 25600;
				ps.ack = 1;
				ps.syn = 0;
				ps.fin = 0;
				ps.data = buffer;
				if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
					perror("ERROR:sending message");
					exit(1);
				} 
				fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);
			}
			begin_process = 0;
		}
		else if(end_process){
			memset((char *) &ps, 0, sizeof(ps));
			ps.seq_num = currSeq;
			ps.ack_num = 0;
			ps.ack = 0;
			ps.syn = 0;
			ps.fin = 1;


			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
				perror("ERROR:sending message");
				exit(1);
			} 
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);
			currSeq += 1;
			if(currSeq == 25601)
				currSeq = 0;


			do
			{
				memset((char *) &pr, 0, sizeof(pr));
				message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
				fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
			}while(pr.ack != 1 && pr.ack_num != currSeq);
			
			//wait 2 seconds while responding to each incoming FIN with ack while dropping other packets
			time_t start, end;
			double elapsed;
			start = time(NULL);

			while(elapsed < 2)
			{
				memset((char *) &ps, 0, sizeof(ps));
				memset((char *) &pr, 0, sizeof(pr));
				end = time(NULL);
				elapsed = difftime(end, start);
				message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
				fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
			
				if(pr.fin == 1)
				{
					//ack
					ps.seq_num = currSeq;
					ps.ack = 1;
					ps.ack_num = pr.seq_num + 1;
					ps.fin = 0;
					ps.syn = 0;

					if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
					{
						perror("ERROR:sending message");
						exit(1);
					} 
					fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);

				}
			}

			exit(0);
		}
		//regular package send
		else{
			message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
			fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
			ps.seq_num = pr.ack_num;
			ps.ack_num = (pr.seq_num + 1) % 25600;
			ps.ack = 1;
			ps.syn = 0;
			ps.fin = 0;
			ps.data = buffer;
			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
				perror("ERROR:sending message");
				exit(1);
			} 
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin);

		}
	


	}
	fclose(fptr);
}


	

	//if(sendto(sockfd, p1, sizeof(p1), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	//{
	//	perror("ERROR:sending message");
	//	exit(1);
	//} 
