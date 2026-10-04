#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct node{
    void* value;
    struct node* next; 
    int holds_item;     // flag used only by waiting list's (cnd_q) nodes. holds_item==1, is when "value" holds the dequeued item. holds_item==0 is when it holds a cnd_t value
} node;

typedef struct queue{
    struct node* back;
    struct node* front;
    size_t total_count;   // # of nodes that were enqueued (including dequeued ones). Used only by normal queue
} queue;

// the queue the assignment requires to implement
queue* q;

// a waiting list implemented using the same queue structure, to perserve FIFO order of threads
queue* cnd_q;   

mtx_t qlock;

void initQueue(void){
    // allocate space for uninitialized queue 
    q = malloc(sizeof(queue));
    q->back = NULL;
    q->front = NULL;
    q->total_count = 0;

    // allocate space for waiting list
    cnd_q = malloc(sizeof(queue));
    cnd_q->back = NULL;
    cnd_q->front = NULL;

    mtx_init(&qlock, mtx_plain);
}

void destroyQueue(void){
    node* n = q->front;
    node* temp;

    // free both the waiting list and the normal queue
    while(n != NULL){
        temp = n;
        n = n->next;
        free(temp);
        // switch to cleaning the waiting list, if normal queue's nodes are freed
        if(n == NULL)
            n = cnd_q->front;
    }

    mtx_destroy(&qlock);

    free(q);
    free(cnd_q);
}  

void enqueue(void* item){
    cnd_t* signal;

    mtx_lock(&qlock);

    // If the waiting list isn't empty, send signal and save item in oldest. Skips actually enqueueing
    if(cnd_q->front != NULL){
        signal = (cnd_t*)cnd_q->front->value;
        // after the cnd_t was saved, we can repurpose the node's "value" field for saving the item, thereby avoiding the enqueue
        cnd_q->front->value = item;
        cnd_q->front->holds_item = 1;

        // dequeue from waiting list
        cnd_q->front = cnd_q->front->next; 
        // if queue is empty, back pointer needs to be updated to NULL as well
        if(cnd_q->front == NULL)
            cnd_q->back = NULL;

        cnd_signal(signal);
    } 
    // otherwise, we can enqueue normally
    else{
        node* n = malloc(sizeof(node));
        n->value = item;
        n->next = NULL;
        // if queue is empty, we need to update the front as well
        if(q->back == NULL){    
            q->front = n;
        }
        else{                   
            q->back->next = n;
        }
        q->back = n;
    }

    mtx_unlock(&qlock);
}

void* dequeue(void){
    void* ret_val;
    node* cnd_node;
    node* temp;

    mtx_lock(&qlock);

    // if the queue is empty, we push a cnd_t special for this call to the waiting list, and wait for enqueue to call it
    if(q->front == NULL){
        // allocate space for the new condition node, and initialize it's value
        cnd_t this_cnd;
        cnd_init(&this_cnd);

        cnd_node = malloc(sizeof(node));
        cnd_node->value = (void*) &this_cnd;
        cnd_node->next = NULL;
        cnd_node->holds_item = 0;

        // enqueue to waiting list queue
        if(cnd_q->back == NULL)
            cnd_q->front = cnd_node;
        else
            cnd_q->back->next = cnd_node;
        cnd_q->back = cnd_node;

        // wait until an item has been passed to it
        while(!cnd_node->holds_item)
            cnd_wait(&this_cnd, &qlock);

        // retrieve the item
        ret_val = cnd_node->value;

        // finished waiting. We can destroy this_cnd
        cnd_destroy(&this_cnd);
        free(cnd_node);
    }
    // if queue isn't empty - dequeue normally
    else{
        temp = q->front;
        q->front = q->front->next;
        ret_val = temp->value;
        free(temp);

        // if queue is empty, back pointer needs to be updated to NULL as well
        if(q->front == NULL)
            q->back = NULL;

    }

    // increment enqueued count
    q->total_count++;

    mtx_unlock(&qlock);
    
    return ret_val;
}

size_t visited(void){
    return q->total_count;
}

