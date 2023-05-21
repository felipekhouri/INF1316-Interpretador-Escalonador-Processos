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
    if (q->front == NULL)
    {
        return TRUE;
    }
    return FALSE;
}

void enqueue(Queue *q, Process p)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->process = p;
    newNode->next = NULL;

    if (isEmpty(q))
    {
        q->front = newNode;
        q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
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

    Node *temp = q->front;
    printf("*******************\n");

    while (temp != NULL)
    {
        // printf("%s\nInício: %d' \nDuração: %d' \n", temp->process.name, temp->process.init, temp->process.duration);
        printf("%s -> ", temp->process.name);
        temp = temp->next;
    }

    printf("FINAL DA FILA\n*******************\n");
}

void swap(Process *a, Process *b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

Node *partition(Node *low, Node *high)
{
    Process pivot = high->process;
    Node *i = low->prev;

    for (Node *j = low; j != high; j = j->next)
    {
        if (j->process.init < pivot.init)
        {
            i = (i == NULL) ? low : i->next;
            swap(&(i->process), &(j->process));
        }
    }

    i = (i == NULL) ? low : i->next;
    swap(&(i->process), &(high->process));
    return i;
}

void quickSort(Node *low, Node *high)
{
    if (high != NULL && low != high && low != high->next)
    {
        Node *pivot = partition(low, high->prev);
        quickSort(low, pivot->prev);
        quickSort(pivot->next, high);
    }
}

void queueSort(Queue *q)
{
    if (isEmpty(q) || q->front->next == NULL)
    {
        return;
    }

    Node *lastNode = q->front;
    while (lastNode->next != NULL)
    {
        lastNode = lastNode->next;
    }

    quickSort(q->front, lastNode);
}
