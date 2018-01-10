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

		 	/* Opening the file */ 

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


			 // week keep a small window size of 5
			unsigned int window_size;
	
				window_size = 5;
			printf("Window Size if %i packets\n", window_size);
			unsigned int curr_window_elem = 0;

			time_t time_to_wait_s = 0;

			// we wait .5 seconds to receive the ACK

				// create a new window and let it know the total number of packets
			suseconds_t time_to_wait_us = 500000;

			window w = generateWindow(window_size, num_packets);

			// Filling the window that we just created untill it returns false

				while (addWindowElement(&w, (file_packets + curr_window_elem))) {
					curr_window_elem++;
				}

				/* Keep looping until all packets have been successfully transfered */

				while (1) {

					window_element* we = getElementFromWindow(w);
					while (we != NULL) {
						unsigned int cs_num = we->packet->seq_num;

						sendto(sockfd, (char *) we->packet, sizeof(char) * PACKET_SIZE, 
										0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));	
						if (we->status == WE_RESEND) {
							printf("Retransmitting packet number %d\n", cs_num);
						}
						if (we->status == WE_NOT_SENT) {
							printf("Transmitting packet number %d\n", cs_num);
						}


						struct timeval tv;
						gettimeofday(&tv, NULL);
						time_t t_s = tv.tv_sec + time_to_wait_s;
						suseconds_t t_us = tv.tv_usec + time_to_wait_us;

						we->tv.tv_sec = t_s;
						we->tv.tv_usec = t_us;
						we->status = WE_SENT;
						we = getElementFromWindow(w);
					}


					//Keep looping until all 5 window elements have been sent
					
					//for the stop & wait protocol, window size = num_packets+1 
					// this while will have another if condition
					// inside that will check if the ACK for this packet has been received or not
					// if not received then go check if timeout has happened, then resend, otherwise
					// just keep looping 

					// for the GoBackN protocol this while loop remains the same	

					

					// Loop to acquire the ACK msg
					
					// for the stop and wait protocol this while loop doesnt exit

					// for the GoBackN protocol 
					// in stop & wait you compare the ACK_msg->seq_num to the sent seq num and resend if not equal	



					bool didreceive = true;
					while (didreceive) {
						didreceive = false;
						if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*) &cli_addr, &clilen) != -1) {
							didreceive = true;
						

							packet* ACK_msg = (packet *) buffer;

							if (ACK_msg == NULL) {
								error("ERROR Nothing in ACK msg buffer");
							}

							int latest_ACK_received = -1;
							if (ACK_msg->type == ACKPACKET)
								latest_ACK_received = ACK_msg->seq_num;
							
							if (ackWindowElement(&w, latest_ACK_received)) {

								printf("ACK for the packet %i received\n", latest_ACK_received);


								// if the first window element is ACK'd, we can slide window
								if (num_packets == curr_window_elem + 1 && w.length == 0) {
									printf("ACK for the last packet received\n");
									break;
								}

							}

							

						}
					} // End of if recv ACK

					cleanWindow(&w);
					while (curr_window_elem != num_packets && addWindowElement(&w, (file_packets + curr_window_elem)))
						curr_window_elem++;

					if (num_packets == curr_window_elem && w.length == 0) {
						printf("ACK for the last packet received\n");
						break;
					}

				} // End of ACK while loop
				printf("Done with file transfer\n\n");
				exit(1);



				bzero((char*) buffer, sizeof(char) * PACKET_SIZE);
		} // end of if(recvfrom)
	}

 	close(sockfd);
 	return 0;

}
