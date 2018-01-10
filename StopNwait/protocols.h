#ifndef SLL
#define SLL

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define WE_NOT_SENT 0
#define WE_RESEND 1
#define WE_SENT 2
#define WE_ACK  3

#define MAX_SEQ_NUM 30

#define ACKPACKET 'a'
#define RETRANSMITPACKET 'r'
#define SENDPACKET 's'
#define FILENOTFOUNDPACKET 'n'
#define FILEREQPACKET 'f'

#define PACKET_SIZE 1024
#define PACKET_CONTENT_SIZE (PACKET_SIZE - sizeof(long) - 3*sizeof(int) - 1)
// -1 for \0 at the end

typedef struct _packet {
    char type;
	unsigned long total_size;
	unsigned int seq_num;
	char buffer[PACKET_CONTENT_SIZE + 1];
	// +1 for \0 at the end
} packet;

typedef struct window_element {
    struct window_element* next;
    packet* packet;
    int status; 
    struct timeval tv;
} window_element;

typedef struct {
    window_element* head;
    window_element* tail;
    int length;
    int max_size;
    int total_packets;
    int current_packets;
} window;

// Generates a new window struct
window generateWindow(int window_size, int num_packets);

// Marks a packet as acknowledged. Should be used when parsing client messages.
bool ackWindowElement(window* w, int sequenceNum);

// Marks packet as needing to be resent. Should be used when parsing cient messages.
bool resendWindowElement(window* w, int sequenceNum);

// Removes elements from the head of the window if they are sent and acknowledged.
void cleanWindow(window* w);

// Add an element to the window. If the window is full, it will return false.
bool addWindowElement(window* w, packet* packet);

// Get first window element that needs to be sent.
window_element* getElementFromWindow(window w);

/* Other auxiliary functions */
bool shouldReceive(float pL, float pC);

window generateWindow(int window_size, int num_packets) {
    window w;
    w.length = 0;
    w.max_size = window_size;
    w.head = NULL;
    w.tail = NULL;
    w.total_packets = num_packets;
    w.current_packets = 0;
    return w;
}

bool setWindowElementStatus(window* w, int sequenceNum, int status) 
{
    window_element* cur = w->head;
    while (cur != NULL) {
        if (cur->packet->seq_num == sequenceNum)
        {
            cur->status = status;
            return true;

        }
        cur = cur->next;
    }
    // Element not found
    return false;
}


bool ackWindowElement(window* w, int sequenceNum) 
{
    return setWindowElementStatus(w, sequenceNum, WE_ACK);
}

void cleanWindow(window* w) 
{
    // If the head of the window can be recycled, it recycles the head and sets the window
    while (w->head != NULL && w->head->status == WE_ACK) {
        w->length--;
        window_element* temp = w->head;

        if (w->head != w->tail) 
            w->head = w->head->next;
        else {
            w->head = NULL;
            w->tail = NULL;
        }
        free(temp);
    }
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    window_element* cur = w->head;

    while (cur != NULL) {
        if (cur->status != WE_ACK && cur->status != WE_NOT_SENT) {
            if (tv.tv_sec > cur->tv.tv_sec || (tv.tv_sec == cur->tv.tv_sec && tv.tv_usec > cur->tv.tv_usec)) {
                cur->status = WE_RESEND;
                cur->packet->type = RETRANSMITPACKET;
                printf("Packet number %d has timed out.\n", cur->packet->seq_num);
            }
        }
        cur = cur->next;
    }

}

bool addWindowElement(window* w, packet* packet)
{
    if (w->length == w->max_size)
        return false;

    if (w->current_packets >= w->total_packets)
        return false;

    w->length++;
    w->current_packets++;
    window_element* nelement = malloc(sizeof(window_element));
    nelement->status = WE_NOT_SENT;
    nelement->packet = packet;
    nelement->next = NULL;
    
    if (w->head == NULL && w->tail == NULL) {
        w->head = nelement;
        w->tail = nelement;
    }
    else {
        w->tail->next = nelement;
        w->tail = nelement;
    } 
   
    return true; 
}

window_element* getElementFromWindow(window w)
{
    window_element* cur = w.head;
    while (cur != NULL)
    {
        if (cur->status == WE_NOT_SENT || cur->status == WE_RESEND)
        {
            return cur;
        }

        cur = cur->next;
    }
    return NULL;
}




#endif
