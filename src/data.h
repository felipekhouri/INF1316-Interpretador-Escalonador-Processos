#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

#define REAL_TIME 0
#define ROUND_ROBIN 1

#define SHM_KEY 1000
#define SHM_KEY2 7000

#define MAX_PROCESSOS 20

typedef struct process
{
    char filename[8];      // Processo a ser executado
    int schedulingAlg;    // 0 = RT & 1 = RR
    int I;    // Inicio do tempo de execução 
    int index;     
    int D;  
    int started;   
    pid_t pid;
} Process;

typedef struct node
{
    Process process;
    struct node *next;
} Node;

typedef struct queue
{
    Node *ahead;
    Node *behind;
} Queue;

void initQueue(Queue *q);
int isEmpty(Queue *q);
void enqueue(Queue *q, Process p);
void dequeue(Queue *q);
Node* merge(Node* left, Node* right);
void split(Node* source, Node** frontRef, Node** backRef);
void mergeSort(Node** headRef);
void printQueue(Queue *q);
void queueSort(Queue *q);
