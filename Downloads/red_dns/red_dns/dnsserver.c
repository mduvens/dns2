#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Should be 53, but when developping I do not want to change my dns server */
#define PORT 15353	

#define MAX_LENGTH 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd;						/* socket */
	int clientlen;					/* byte size of client's address */
	struct sockaddr_in serveraddr;	/* server's addr */
	struct sockaddr_in clientaddr;	/* client addr */
	int optval;						/* flag value for setsockopt */
	int len;						/* message byte size */


	/* 
	 * socket: create the parent socket 
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		   (const void *)&optval, sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
		 sizeof(serveraddr)) < 0)
		error("ERROR on binding");

	/* 
	 * main loop: wait for a datagram, then echo it
	 */
	clientlen = sizeof(clientaddr);
	char *udp_payload = malloc(MAX_LENGTH * sizeof(char));   // Allocate and set to 0
	while (1) {

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		memset(udp_payload, 0, MAX_LENGTH);
		len = recvfrom(sockfd, udp_payload, MAX_LENGTH, 0,
			     (struct sockaddr *)&clientaddr, &clientlen);
		if (len < 0)
			error("ERROR in recvfrom");

	    printf("Received payload:");
	    for (int i = 0; i < len; i++) {
	        if ((i % 8) == 0) printf("\t");
	        if ((i % 16) == 0) printf("\n");
	        printf(" %02X", (unsigned char)udp_payload[i]);
	    }
	    printf("\n\n");
	    fflush(stdout);

		/* 
		 * sendto: echo the input back to the client 
		 */
		len = sendto(sockfd, udp_payload, len, 0,
			   (struct sockaddr *)&clientaddr, clientlen);
		if (len < 0)
			error("ERROR in sendto");
	}
}
