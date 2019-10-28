#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DATA_BUFFER_LEN (1024*12)
typedef enum { 
	READABLE = 0, 
	WRITABLE = 1, 
	UNKNOWN = 255
}ACCESS_FLAG ;

typedef struct cmd_pack 
{
	char cmd_data[DATA_BUFFER_LEN];
	ACCESS_FLAG flag;
}CmdPack, *PCmdPack;

typedef struct queue {
    CmdPack* pdata;
    int front;
    int rear;
    int max_size;
}queue, *pqueue;

void init_queue(pqueue q, int max_size);
int is_full_queue(pqueue q);
int is_empty_queue(pqueue q);
int enqueue(pqueue q, char* pdata);
int dequeue(pqueue q, char* pdata);
void print_queue(pqueue q);





#endif

