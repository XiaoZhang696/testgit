#include "queue.h"
#include "../Common/Log.h"

void init_queue(pqueue q, int max_size)
{
	int i = 0;
	q->front = q->rear = 0;
	q->pdata = (CmdPack *)malloc(max_size * sizeof(CmdPack));
	q->max_size = max_size;
	for(i = q->front; i < q->max_size; i++){
		q->pdata[i].flag= WRITABLE;
	}
	ZDBG("queue create successfully!\n");
	return;
}

int is_full_queue(pqueue q)
{
    return (q->front == (q->rear+1)%q->max_size) ? 1 : 0;
}

int is_empty_queue(pqueue q)
{
    return (q->front == q->rear) ? 1 : 0;
}

int enqueue(pqueue q, char* pval)
{
	if(is_full_queue(q)){
		ZDBG("is_full_queue.\n");
		return 0;
	}else{
		if( (q->pdata[q->rear].flag!=WRITABLE) && (q->pdata[q->rear].flag!=UNKNOWN)){
			ZDBG("flag is not WRITABLE.\n");
			return 0;
		}
		//ZDBG("%s: %s\n", __func__, pval);
		//memset(q->pdata[q->rear].cmd_data, '\0', DATA_BUFFER_LEN*sizeof(char));
		//sprintf(q->pdata[q->rear].cmd_data, "%s", pval);
		bzero(q->pdata[q->rear].cmd_data, DATA_BUFFER_LEN);
		snprintf(q->pdata[q->rear].cmd_data, DATA_BUFFER_LEN, "%s", pval);
		q->pdata[q->rear].flag = READABLE;
		//ZDBG("enqueue: %s\n", pval);
		q->rear = (q->rear+1)%q->max_size;
    	}
	return 1;
}

int dequeue(pqueue q, char* pval)
{
	if(is_empty_queue(q)){
		ZDBG("is_empty_queue\n");
		return 0;
	}else{
		if( (q->pdata[q->front].flag!=READABLE) && (q->pdata[q->rear].flag!=UNKNOWN)){
			return 0;
		}		
		//sprintf(pval, "%s", q->pdata[q->front].cmd_data);
		memcpy(pval, q->pdata[q->front].cmd_data, strlen(q->pdata[q->front].cmd_data));
		q->pdata[q->front].flag = WRITABLE;
		
		q->front = (q->front+1)%q->max_size;
		//ZDBG("dequeue: %s\n", pval);
	}
	return 1;
}

void print_queue(pqueue q)
{
	int i = 0;
	if(is_empty_queue(q)) {
		ZDBG("queue is empty\n");
		return ;
	}else{
		for(i = q->front; i < q->rear; i++){
			ZDBG("queue data is %s\n", q->pdata[i].cmd_data);
		}
	}
	return;
}
