#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "queue.h"

typedef struct queue
{
    Node *ahead;
    Node *behind;
} Queue;


void initQueue(Queue *q);
int isQueueEmpty(Queue *q);
void enqueue(Queue *q, Process p);
void dequeue(Queue *q);
Node* merge(Node* left, Node* right);
void split(Node* source, Node** frontRef, Node** backRef);
void mergeSort(Node** headRef);
void printQueue(Queue *q);
void queueSort(Queue *q);
