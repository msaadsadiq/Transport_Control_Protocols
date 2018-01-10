#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include "protocols.h"


void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	int sockfd, newsockfd, portno, pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	char buffer[PACKET_SIZE];

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}


	//reset memory
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	//fill in address info
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	//create UDP socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	error("ERROR opening socket");

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	error("ERROR on binding");

	bzero((char*) &cli_addr, sizeof(cli_addr));
	clilen = sizeof(cli_addr);

	// wait for connection
	printf("Waiting for receiver\n\n");
	bzero((char*) buffer, sizeof(char) * PACKET_SIZE);



	while (1) {
		// once we receive secret key from receiver, we go in. Otherwise, loop to keep listening
		if ((recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &clilen) != -1) && (strcmp(buffer, "Communications") == 0))
		{

			printf("Secret Pass key Matched! \n");

			int namelen = strlen(buffer);
			char filename[namelen + 1];
			bzero(filename, sizeof(char) * (namelen + 1));
			filename[namelen] = '\0';
			strncpy(filename, argv[2], namelen);

			bzero((char*) buffer, sizeof(char) * PACKET_SIZE);

			printf("Sending  %s  to the Receiver \n\n", filename);

			/*	Opening the File */

			FILE* f =  fopen(filename, "r");
			if (f == NULL) {
				packet notfound;
				notfound.type = FILENOTFOUNDPACKET;
				notfound.total_size = 0;
				printf("ERROR opening requested file\n");
				sendto(sockfd, (char *) &notfound, sizeof(char) * PACKET_SIZE,
				0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
				continue;

			}

			// calculating the number of bytes in the file
			struct stat st;
			stat(filename, &st);
			int fileBytes = st.st_size;

			char* fileContent = (char*) calloc(fileBytes, sizeof(char));
			if (fileContent == NULL) {
				error("ERROR allocating space for file");
			}

			fread(fileContent, sizeof(char), fileBytes, f);
			fclose(f);

			/* 	Breaking down the file into packets */

			int num_packets = fileBytes / PACKET_CONTENT_SIZE;
			int remainderBytes = fileBytes % PACKET_CONTENT_SIZE;
			if (remainderBytes)
			num_packets++;

			printf("Number of packets for the file %s is: %i\n", filename, num_packets);

			/* Creating the Packet Linked List */

			packet* file_packets;
			file_packets = (packet *) calloc(num_packets, PACKET_SIZE);

			printf("Packet Array initialized\n");

			int i;
			for (i = 0; i < num_packets; i++) {

				if ((i == num_packets - 1) && remainderBytes) {
					// the last packet with remainder bytes doesn't take full space
					strncpy(file_packets[i].buffer, fileContent + i * PACKET_CONTENT_SIZE, remainderBytes);
					file_packets[i].buffer[remainderBytes] = '\0';
				}
				else { // normal cases
					strncpy(file_packets[i].buffer, fileContent + i * PACKET_CONTENT_SIZE, PACKET_CONTENT_SIZE);
					file_packets[i].buffer[PACKET_CONTENT_SIZE]	= '\0';
				}

				file_packets[i].total_size = fileBytes;
				file_packets[i].seq_num = (i % MAX_SEQ_NUM)*1024;
				file_packets[i].type = SENDPACKET;
			}

			printf("Created array of packets with %i packets\n\n", num_packets);


			/* Sending Packets to the receiver*/


			// In the stop and wait, we keep the window size to 1. So effectively we reuse our code for Go-back-N

			unsigned int window_size;
			window_size = 1;
			unsigned int curr_window_elem = 0;

			time_t time_to_wait_s = 0;
			suseconds_t time_to_wait_us = 500000;


			// we wait 1 seconds to receive the ACK

			// create a new window and let it know the total number of packets
			// here, packet_to_send will be one greater than last_window_packet

			/*
			Loop to receive ACKs within the window
			*/

			i=0;

			while ( file_packets[i].seq_num/1024 < num_packets+1)
			{

				printf("Transmitting packet number %d\n", file_packets[i].seq_num);

				sendto(sockfd, (char *) file_packets, sizeof(char) * PACKET_SIZE,
				0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));

				bool didreceive = true;

				while (didreceive)
				{

					struct timeval tv;
					gettimeofday(&tv, NULL);

					if (tv.tv_usec > time_to_wait_us)
					{
						printf("Packet number %d has timed out.\n", file_packets[i].seq_num);
						printf("Retransmitting packet number %d\n", file_packets[i].seq_num);
						file_packets[i].type = RETRANSMITPACKET;
						sendto(sockfd, (char *) file_packets, sizeof(char) * PACKET_SIZE,
						0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));

					}

					if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*) &cli_addr, &clilen) != -1)
					{


						printf("ACK for the packet %d received\n",file_packets[i].seq_num);
						i++;
						curr_window_elem++;
						didreceive = false;
					}

					// Check for time out

				}



				if (num_packets == curr_window_elem)
				{
					printf("ACK for the last packet received\n");
					break;
				}
			}




			// End of ACK while loop
			printf("Done with file transfer\n\n");
			exit(1);



			bzero((char*) buffer, sizeof(char) * PACKET_SIZE);
		} // end of if(recvfrom)
	}

	close(sockfd);
	return 0;

}
