#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct node{
    void* value;
    struct node* next;  
} node;

typedef struct queue{
    struct node* back;
    struct node* front;
    // # of nodes that were enqueued (including dequeued ones)
    size_t total_count;   
} queue;



queue* q;
queue* cnd_q;


mtx_t qlock;
cnd_t notEmpty;



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
    cnd_init(&notEmpty);
}

void destroyQueue(void){
    node* n = q->front;
    node* temp;

    while(n != NULL){
        temp = n;
        n = n->next;
        free(temp);
        // switch to clean waiting list, if normal queue's nodes are freed
        if(n == NULL)
            n = cnd_q->front;
    }

    mtx_destroy(&qlock);
    cnd_destroy(&notEmpty);

    free(q);
    free(cnd_q);
}  

void enqueue(void* item){
    node* temp_cnd_node;
    cnd_t* dequeued_cnd;

    mtx_lock(&qlock);

    node* n = malloc(sizeof(node));
    n->value = item;
    n->next = NULL;
    if(q->back == NULL){    // if queue is empty, we need to update the front as well
        q->front = n;
    }
    else{                   
        q->back->next = n;
    }
    q->back = n;

    // dequeue the oldest cnd_t from the waiting list
    if(cnd_q->front != NULL){
        temp_cnd_node = cnd_q->front;
        cnd_q->front = cnd_q->front->next;
        dequeued_cnd = (cnd_t*)temp_cnd_node->value;
        free(temp_cnd_node);

        if(cnd_q->front == NULL)
            cnd_q->back = NULL;

        cnd_signal(dequeued_cnd);
    }
    
    // cnd_signal(&notEmpty);  // X (delete) -> dequeue FIFO order signal, and call cnd_signal for it
    mtx_unlock(&qlock);
}

void* dequeue(void){
    void* ret_val;
    node* cnd_node;
    node* temp;

    mtx_lock(&qlock);

    // if the queue is empty, we push a cnd_t special for this call to the waiting list, and wait for enqueue to call it
    while(q->front == NULL){
        // allocate space for the new condition node, and it's value
        cnd_t* this_cnd = malloc(sizeof(cnd_t));
        cnd_node = malloc(sizeof(node));

        cnd_init(this_cnd);

        cnd_node->value = (void*) this_cnd;
        cnd_node->next = NULL;

        // enqueue to conditions queue
        if(cnd_q->back == NULL){
            cnd_q->front = cnd_node;
        }
        else{
            cnd_q->back->next = cnd_node;
        }
        cnd_q->back = cnd_node;

        // wait for signal 
        cnd_wait(this_cnd, &qlock);

        // finished waiting. We can destroy this_cnd
        cnd_destroy(this_cnd);
        free(this_cnd);
    }
    
    /*
    while(q->front == NULL)
        cnd_wait(&notEmpty,&qlock); // X (delete) -> wait for the relevant condition
    */

    temp = q->front;
    q->front = q->front->next;
    ret_val = temp->value;
    free(temp);

    // if queue is empty, back pointer needs to be updated to NULL as well
    if(q->front == NULL)
        q->back = NULL;

    // increment enqueued count
    q->total_count++;

    mtx_unlock(&qlock);
    
    return ret_val;
}

size_t visited(void){
    return q->total_count;
}
