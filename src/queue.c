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

/*
    A função "enqueue" adiciona um novo processo à fila.
    Ela cria um novo nó para armazenar o processo e o adiciona ao final da fila.
    Se a fila estiver vazia, o novo nó é tanto o primeiro quanto o último da fila.
    Caso contrário, o novo nó é adicionado após o último nó existente e se torna o novo último da fila.
*/
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




/*
    As funções "merge", "split" e "mergeSort" implementam o algoritmo de ordenação merge sort na fila.
    Elas são utilizadas para ordenar a fila com base no instante de início dos processos.
    A função "merge" recebe dois nós e os mescla em ordem crescente com base no instante de início.
    A função "split" divide a lista em duas metades aproximadamente iguais.
    A função "mergeSort" chama a si mesma recursivamente para ordenar as metades esquerda e direita da lista.
    A função "queueSort" é a função de interface para chamar o merge sort na fila.
    Ela verifica se a fila está vazia ou se contém apenas um elemento antes de chamar o merge sort.
*/
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


/*
    A função "dequeue" remove o primeiro elemento da fila.
    Se a fila estiver vazia, exibe uma mensagem de erro.
    Caso contrário, atualiza o ponteiro "front" para o próximo elemento da fila.
    Se o elemento removido for o último da fila, o ponteiro "rear" também é atualizado para NULL,
    indicando que a fila está vazia.
    Por fim, libera a memória ocupada pelo antigo primeiro elemento.
*/

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



/*
    A função "displayQueue" exibe o conteúdo da fila.
    Se a fila estiver vazia, exibe uma mensagem informando que a fila está vazia.
    Caso contrário, percorre a fila e imprime o nome de cada processo seguido por uma seta "->".
    Por fim, exibe a indicação de final da fila.
*/

void displayQueue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("Fila vazia.\n");
        return;
    }

    for (Node *temp = q->front; temp != NULL; temp = temp->next)
    {
        printf("->%s", temp->process.name);
    }
}

