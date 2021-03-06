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
#define MAXCONG 20

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

int main(int argc, char **argv)
{

	int sockfd;
	char buffer[PAYLOAD] = {'\0'};
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

	int cwnd = 1;
	int ssthresh = 10;

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

	currSeq = ps.seq_num;
	if(currSeq == 25600)
		currSeq = 0;

	currAck = 0;
	int buflen;
	int end_process = 0;
	int begin_process = 1;
	int redo = 0;

	struct timeval timeout = {10, 0};

	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)) < 0)
	{
		perror("ERROR:setting timeout\n");
	}

	fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);

	message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);

	fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);

	memset((char *) &ps, 0, sizeof(ps));


	fptr = fopen(filename, "r");

	if (fptr == NULL)
	{
		perror("ERROR:opening file\n");
		exit(1);
	}

	fseek(fptr, 0, SEEK_SET);
	memset((char *) &buffer, '\0', sizeof(buffer));
	buflen = fread(ps.data, 1,PAYLOAD, fptr);
	if (buflen < 1) {
        if (!feof(fptr)) {
            perror("ERROR:reading file\n");
			exit(1);
        }
        else{
        	end_process = 1;
        }
    }

    ps.seq_num = pr.ack_num;
    ps.ack_num = (pr.seq_num + 1) % 25600;
    currSeq = ps.seq_num;
    currAck = ps.ack_num;
    ps.ack = 1;
    ps.syn = 0;
    ps.fin = 0;
    ps.size = buflen;

    if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
    	perror("ERROR:sending message");
    	exit(1);
    } 
    fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);



    int firstiter = 1;
    int16_t prev_ack = -1;
    int dup_count = 0;

    struct timeval start1, end1;
	gettimeofday(&start1, NULL);
	struct timeval timeout3 = {1, 0};
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout3, sizeof(struct timeval)) < 0)
	{
		perror("ERROR:setting timeout\n");
	}

	while(1){
		memset((char *) &ps, 0, sizeof(ps));
		memset((char *) &pr, 0, sizeof(pr));
		if(!firstiter){
			gettimeofday(&end1, NULL);
	        if(end1.tv_sec - start1.tv_sec > 0.5){
	            ssthresh = 2;
				if ((cwnd/2)>2){
					ssthresh = cwnd/2;
				}
				cwnd = 1;
				if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
					perror("ERROR:sending message");
					exit(1);
				} 
				fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
	        }
		}
		else{
			firstiter = 0;
		}
		message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
		gettimeofday(&start1, NULL);
		if(prev_ack == pr.ack_num){
			dup_count++;
			if (dup_count == 3){
				ssthresh = 2;
				if ((cwnd/2)>2){
					ssthresh = cwnd/2;
				}
				cwnd = ssthresh + 3;
			}
			else if(dup_count > 3){
				cwnd += 1;
			}

		}
		else if(dup_count >= 3 && prev_ack != pr.ack_num){
			cwnd = ssthresh;
		}
		else{
			prev_ack = pr.ack_num;
			dup_count = 0;
		}
		fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
		if(pr.ack == 1){
			if(cwnd < ssthresh){
				cwnd += 1;
			}
			else{
				cwnd = cwnd + (1/cwnd);
			}
		}

		if(end_process){
			memset((char *) &ps, 0, sizeof(ps));
			ps.seq_num = currSeq + 1;
			ps.ack_num = 0;
			ps.ack = 0;
			ps.syn = 0;
			ps.fin = 1;
			ps.size = 0;
			//ps.data = {0};

			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
				perror("ERROR:sending message");
				exit(1);
			} 
			currSeq += 1;
			if(currSeq == 25601)
				currSeq = 0;

			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);

			do
			{
				memset((char *) &pr, 0, sizeof(pr));
				message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
				fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
			}while(pr.ack != 1 && pr.ack_num != currSeq);
			
			//wait 2 seconds while responding to each incoming FIN with ack while dropping other packets
			int leave = 1;
			struct timeval start, end;
			gettimeofday(&start, NULL);
			struct timeval timeout2 = {1, 0};
			if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout2, sizeof(struct timeval)) < 0)
			{
				perror("ERROR:setting timeout\n");
			}
			
			while(leave)
			{
				gettimeofday(&end, NULL);
				if(end.tv_sec - start.tv_sec > 2)
					leave = 0;
				memset((char *) &ps, 0, sizeof(ps));
				memset((char *) &pr, 0, sizeof(pr));
				message_size = recvfrom(sockfd, &pr, sizeof(pr), 0, (struct sockaddr *)&serveraddr, &addr_len);
				if(message_size > 0)
				{
					fprintf(stdout, "RECV %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 "\n", pr.seq_num, pr.ack_num, cwnd, ssthresh, pr.ack, pr.syn, pr.fin);
					if(pr.fin == 1)
					{
						//ack
						ps.seq_num = currSeq;
						ps.ack = 1;
						ps.ack_num = pr.seq_num + 1;
						ps.fin = 0;
						ps.syn = 0;
						ps.size = 0;

						if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
						{
							perror("ERROR:sending message");
							exit(1);
						} 
					}
				}
				
			}
			//break;

			exit(0);
		}
		else if ((((currSeq + buflen)%25600) == pr.ack_num) && (currAck == pr.seq_num)){
			memset((char *) &ps, 0, sizeof(ps));
			memset((char *) &buffer, '\0', sizeof(buffer));
			buflen = fread(ps.data, 1,PAYLOAD, fptr);
			if (buflen < 1) {
		        if (!feof(fptr)) {
		            perror("ERROR:reading file\n");
					exit(1);
		        }
		        else{
		        	end_process = 1;
		        	continue;
		        }
		    }
			ps.seq_num = pr.ack_num;
			ps.ack_num = (pr.seq_num + 1) % 25600;
			currSeq = ps.seq_num;
			currAck = ps.ack_num;
			ps.ack = 1;
			ps.syn = 0;
			ps.fin = 0;
			ps.size = buflen;
			//ps.data = buffer;
			

			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
				perror("ERROR:sending message");
				exit(1);
			} 
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
		}
		else{
			if(sendto(sockfd, &ps, sizeof(ps), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
			perror("ERROR:sending message");
			exit(1);
			} 
			fprintf(stdout, "SEND %" PRId16 " %" PRId16 " %" PRId16 " %" PRId16 " %" PRId8 " %" PRId8 " %" PRId8 " %" PRId8 "\n", ps.seq_num, ps.ack_num, cwnd, ssthresh, ps.ack, ps.syn, ps.fin,ps.dup);
		}

	}
	fclose(fptr);
}


	

	//if(sendto(sockfd, p1, sizeof(p1), 0, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	//{
	//	perror("ERROR:sending message");
	//	exit(1);
	//} 
