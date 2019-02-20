#include<stdio.h>
#include<stdlib.h>

//queue element structure
struct qnode{

    int key;
    struct qnode *next;    
};

struct queue{
    struct qnode *head, *tail;
};

struct qnode* create_node(int k){

   struct qnode *new_node = (struct qnode*)malloc(sizeof(struct qnode));
   new_node-> key = k;
   new_node->next = NULL;
   return new_node;
}

struct queue *create_queue(){

    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    q->head = q->tail = NULL;
    return q;
}

void enqueue(struct queue *q, int k){

    //create new node
    struct qnode *temp = create_node(k);

    //if queue is empty
    if(q->tail == NULL){
        q->head = q->tail = temp;
        return;
    }

    //add new node at end
    q->tail->next = temp;
    q->tail = temp;    
}

struct qnode* dequeue(struct queue *q){

    //if queue empty, return
    if(q->head == NULL)
        return NULL;
    
    struct qnode *temp = q->head;
    q->head = q->head->next;

    if(q->head == NULL){
        q->tail = NULL;
    }
    return temp;
}

int peek(struct queue *q){

    if(q->head == NULL)
        return -1;
    
    return q->head->key;
}

int main(){

    struct queue *q = create_queue();

    enqueue(q,10);
    enqueue(q,20);

    struct qnode *x = dequeue(q);
    printf("%d\n", x->key);
    printf("%d\n",peek(q));
    x = dequeue(q);
    printf("%d\n",x->key);
    printf("%d\n",peek(q));
    x = dequeue(q);
}
