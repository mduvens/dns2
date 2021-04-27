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
	struct sockaddr_in serveraddr;	/* server's addr */
	struct sockaddr_in clientaddr;	/* client addr */
	int optval;						/* flag value for setsockopt */
	int len;						/* message byte size */

	// Data sections
	char udp_payload[MAX_LENGTH]; 	/* UDP Payload */
	unsigned char control[CMSG_SPACE(sizeof(struct in_pktinfo))];	/* Control messages */


	// Related to getting destination IP from received packets
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov[1];

	struct in_pktinfo *pktinfo;		/* To set the source IP address in a response */

	unsigned int interface_idx;		// Interface index of the received packet
	struct in_addr recv_dest_addr;	// Destination address of the received packet

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
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int))  == -1)
		error("setsockopt - reuse address");
	
	/* Allow us to retrieve the received packets' destination address */
	if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, (const void *)&optval, sizeof(int))  == -1)
		error("setsockopt - ip packet info");

	/*
	 * build the server's Internet address
	 * We listen to all addresses and send the response from the received packets
	 * destination address, so it should always be accepted.
	 */
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in )) == -1)
		error("ERROR on binding");

	iov[0].iov_base = udp_payload;
	iov[0].iov_len = MAX_LENGTH;		// On receive set to max buffer length

	/* Tricky header stuff for more control */
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_name = &clientaddr;
	msg.msg_namelen = sizeof(struct sockaddr_in);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);


	/* 
	 * main loop: wait for a datagram, then echo it
	 */
	while (1) {
		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		memset(udp_payload, 0, MAX_LENGTH);
		interface_idx = (unsigned int) -1;	// So we can check for the correct one

		len = recvmsg(sockfd, &msg, 0);
		if (len < 0)
			error("ERROR in recvfrom");


		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if ((cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_PKTINFO)) {
				pktinfo = (struct in_pktinfo *)(CMSG_DATA(cmsg));
				interface_idx = pktinfo->ipi_ifindex;
				recv_dest_addr = pktinfo->ipi_addr;
			}
		}

		/*
		 * NOTE: A lot of times these helper functions, like inet_ntoa,
		 * use a static buffer. This means any call to inet_ntoa will
		 * change the static buffer, so you can only use it once in 
		 * every printf!
		 */
		printf("Received payload (%ld bytes) from %s:%hu",
			(long)len,
			inet_ntoa(clientaddr.sin_addr), 
			ntohs(clientaddr.sin_port));
		printf(" to %s via interface %u:\n", 
			inet_ntoa(recv_dest_addr),
			interface_idx);

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

		iov[0].iov_len = len;	// Set the payload size to the number of bytes to send

		// Set the correct source address
	    cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_PKTINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
		pktinfo = (struct in_pktinfo*) CMSG_DATA(cmsg);
		pktinfo->ipi_ifindex = interface_idx;
		pktinfo->ipi_spec_dst = recv_dest_addr;
		len = sendmsg(sockfd, &msg, 0);
		if (len < 0)
			error("ERROR in sendmsg");
	}
}
