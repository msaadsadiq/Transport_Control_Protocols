#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "protocols.h"

void error(char *msg)
{
    perror(msg);
    exit(0);
}

bool receivedAll(bool* checklist, unsigned int csize) {
	// check last window
	int i;
	if (csize < 100) {
		for (i = 0; i < csize; i++)
			if (checklist[i] == false)
				return false;
	}
	else {
		for (i = csize - 100; i < csize; i++)
			if (checklist[i] == false)
				return false;
	}

	return true;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	int clientsocket; //Socket descriptor
    int portno, n;
    socklen_t len;
    struct sockaddr_in serv_addr;
    struct hostent *server; 
    char buffer[PACKET_SIZE];

	if (argc < 3) {
		 fprintf(stderr,"ERROR, no host or port provided\n");
		 exit(1);
	}


    portno = atoi(argv[2]);

	clientsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientsocket < 0)
		error("ERROR opening client socket");


 	bzero((char*) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	serv_addr.sin_port = htons(portno);

 	server = gethostbyname(argv[1]);
 	if (server == NULL) {
 		error("ERROR getting host");
 	}

	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length); 	
	
	printf("Sending request to the sender\n\n");

	// int sendto(int sockfd, const void *msg, int len, unsigned int flags, const struct sockaddr *to, socklen_t tolen);
	// int recvfrom(int sockfd, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen); 

	char* secret_key = argv[3];
	printf("The secret passkey : %s\n", secret_key);
	sendto(clientsocket, secret_key, strlen(secret_key) * sizeof(char), 
					0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));


	// variables to keep track of packet information
	packet* file_packets = NULL;
	unsigned long file_size;
	unsigned int received_packets = 0;
	unsigned int total_num_packets;
	bool firstPacketReceived = false;
	time_t now = time(NULL);

	

	bool* receive_check;

	// variables to deal with > 30k seq num
	unsigned int mult_counter = 0;
	bool shouldAdd = false;
	unsigned int add_counter = 0;
	int last_seq_num = 0;


	// Keeps attempting to send Pass key request until it gets a response. 
	while (firstPacketReceived == false) {
		// Attempts to send a request 
		if (time(NULL) - now > 1) {
				now = time(NULL);
				printf("Sending the Secret Code: %s\n", secret_key);
				sendto(clientsocket, secret_key, strlen(secret_key) * sizeof(char), 
							0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		}

		if (recvfrom(clientsocket, buffer, sizeof(buffer), MSG_DONTWAIT,(struct sockaddr*) &serv_addr, &len) != -1) 
		{
			firstPacketReceived = true;
			packet* content_packet = (packet *) buffer;
			packet ACK_packet;

			int sequenceNum = (content_packet->seq_num)/1024;
			if (last_seq_num - sequenceNum > 20 && !shouldAdd) {
				mult_counter++;
				add_counter = 0;
				shouldAdd = true;
				sequenceNum += mult_counter * MAX_SEQ_NUM;
			}
			else {
				if (sequenceNum > 20 && shouldAdd && add_counter < 15) {
					sequenceNum += (mult_counter - 1) * MAX_SEQ_NUM;
				}
				else {
					sequenceNum += mult_counter * MAX_SEQ_NUM;
					if (shouldAdd)
						add_counter++;
					if (add_counter > 14) {
						shouldAdd = false;
						add_counter = 0;
					}
				}

			}



			last_seq_num = content_packet->seq_num/1024;
			char packetType = content_packet->type;
			if (packetType == SENDPACKET) {
				printf("Packet type: data packet.\n");
			}
			if (packetType == RETRANSMITPACKET) {
				printf("Packet type: retransmitted data packet.\n");
			}
			if (packetType == FILENOTFOUNDPACKET) {
				printf("Packet type: FILE NOT FOUND. Exiting.\n");
				exit(1);
			}


			// Allocates space for file in a file_packet buffer.
			file_size = content_packet->total_size;
			total_num_packets = (file_size / PACKET_CONTENT_SIZE);
			if (file_size % PACKET_CONTENT_SIZE) // dangling byte packet
				total_num_packets++;
			file_packets =  (packet *) calloc(total_num_packets, sizeof(packet));
			if (file_packets == NULL) {
				error("ERROR allocating for receiving file packets");
			}
			else
				printf("Allocated space for %i packets\n", total_num_packets);

			receive_check = (bool *) calloc(total_num_packets, sizeof(bool));

			// Places the first packet received in the correct position of the file_packets buffer.
			
			file_packets[sequenceNum] = *content_packet;
			printf("Got packet number %i. \n", sequenceNum);
			

			received_packets++;
			receive_check[sequenceNum] = true;
			ACK_packet.type = ACKPACKET;
			ACK_packet.seq_num = sequenceNum;
			ACK_packet.total_size = file_size;

			sendto(clientsocket, (char *) &ACK_packet, PACKET_SIZE, 
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			printf("Sent ACK seqnum %i\n\n", ACK_packet.seq_num);

			if (receivedAll(receive_check, total_num_packets)) {
				break;
			}
			
		}
	}


	while (!receivedAll(receive_check, total_num_packets)) {
		// keep looping to receive file packets
		if (recvfrom(clientsocket, buffer, sizeof(buffer), 0,(struct sockaddr*) &serv_addr, &len) != -1) 
		{
			packet* content_packet = (packet *) buffer;
			packet ACK_packet;

			int sequenceNum = (content_packet->seq_num)/1024;

			if (last_seq_num - sequenceNum > 20 && !shouldAdd) {
				mult_counter++;
				add_counter = 0;
				shouldAdd = true;
				sequenceNum += mult_counter * MAX_SEQ_NUM;
			}
			else {
				if (sequenceNum > 20 && shouldAdd && add_counter < 16) {
					sequenceNum += (mult_counter - 1) * MAX_SEQ_NUM;
				}
				else {
					sequenceNum += mult_counter * MAX_SEQ_NUM;
					if (shouldAdd)
						add_counter++;
					if (add_counter > 12) {
						shouldAdd = false;
						add_counter = 0;
					}
				}

			}

			last_seq_num = content_packet->seq_num/1024;

			char packetType = content_packet->type;
			file_packets[sequenceNum] = *content_packet;
			printf("Got packet number %i\n", content_packet->seq_num);
			if (packetType == SENDPACKET) {
				printf("Packet type: data packet.\n");
			}
			if (packetType == RETRANSMITPACKET) {
				printf("Packet type: retransmitted data packet.\n");
			}

			received_packets++;
			receive_check[sequenceNum] = true;
			ACK_packet.type = ACKPACKET;
			ACK_packet.seq_num = content_packet->seq_num; //sequenceNum;// % MAX_SEQ_NUM;
			ACK_packet.total_size = file_size;

			/*
				Received file packet, so send an ACK correspondingly
			*/

			sendto(clientsocket, (char *) &ACK_packet, PACKET_SIZE, 
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			printf("Sent ACK seqnum %i\n\n", ACK_packet.seq_num);

			//if (next_packet > total_num_packets) {
			//	printf("Got the last packet\n");
			//	break;
			//}
		}
	} // End of receiving file packets while loop

	printf("Done receiving file packets and sending ACKs back\n");




	/*
		Write the received packets into file
	*/
	//char fileContent[file_size + 1];
	char* fileContent;
	fileContent = (char *) calloc(file_size + 1, sizeof(char));
	//memset(fileContent, 0, file_size + 1);	


	int i;
	for (i = 0; i < total_num_packets; i++) {
		strcat(fileContent, file_packets[i].buffer);
	}

	printf("FILE COPY DONE - Exiting Gracefully \n");

	fileContent[file_size] = '\0';

	FILE* f = fopen("Received.txt", "wb");
	if (f == NULL) {
		error("ERROR with opening file");
	}

	fwrite(fileContent, sizeof(char), file_size, f);

	fclose(f);


	close(clientsocket);

	return 0;
}


