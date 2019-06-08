default: server client

server: server.c
	gcc server.c -o server

client: client.c
	gcc client.c -o client
	
dist:
	tar -czvf project2_504827264_804809254.tar.gz client.c server.c Makefile README Project2_report.pdf
clean:
	rm -f server client 
