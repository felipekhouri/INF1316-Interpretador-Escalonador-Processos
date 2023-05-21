#include <stdio.h>
#include <stdlib.h>

#include "data.h"

void initQueue(Queue *q)
{
    q->front = NULL;
    q->rear = NULL;
}

int isEmpty(Queue *q)
{
    if (q->front != NULL)
    {
        return FALSE;
    }
    return TRUE;
}

void enqueue(Queue *q, Process p)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->process = p;
    newNode->next = NULL;

    if (isEmpty(q))
    {
        q->front = newNode;
    }
    else
    {
        q->rear->next = newNode;
    }

    q->rear = newNode;
}


Node* merge(Node* left, Node* right)
{
    if (left == NULL)
        return right;
    if (right == NULL)
        return left;

    Node* result = NULL;

    if (left->process.init <= right->process.init) {
        result = left;
        result->next = merge(left->next, right);
    }
    else {
        result = right;
        result->next = merge(left, right->next);
    }

    return result;
}

void split(Node* source, Node** frontRef, Node** backRef)
{
    Node* slow = source;
    Node* fast = source->next;

    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
}

void mergeSort(Node** headRef)
{
    Node* head = *headRef;
    Node* a;
    Node* b;

    if (head == NULL || head->next == NULL)
        return;

    split(head, &a, &b);

    mergeSort(&a);
    mergeSort(&b);

    *headRef = merge(a, b);
}

void queueSort(Queue* q)
{
    if (isEmpty(q) || q->front->next == NULL)
        return;

    mergeSort(&q->front);
}


void dequeue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("A fila está vazia. Nenhum elemento para remover.\n");
        return;
    }

    Node *temp = q->front;
    q->front = q->front->next; // Segundo da fila

    if (q->front == NULL)
    {                   // Se a fila só tinha um elemento
        q->rear = NULL; // Fila se torna vazia
    }

    free(temp); // Libera o antigo primeiro da fila
}

void displayQueue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("A fila está vazia.\n");
        return;
    }

    printf("*******************\n");

    for (Node *temp = q->front; temp != NULL; temp = temp->next)
    {
        printf("%s -> ", temp->process.name);
    }

    printf("FINAL DA FILA\n*******************\n");
}

