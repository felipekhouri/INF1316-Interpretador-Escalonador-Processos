#include <stdio.h>
#include <stdlib.h>

#include "data.h"

void initQueue(Queue *q)
{
    q->ahead = NULL;
    q->behind = NULL;
}

int isEmpty(Queue *q)
{
    if (q->ahead != NULL)
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
        q->ahead = newNode;
    }
    else
    {
        q->behind->next = newNode;
    }

    q->behind = newNode;
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

    Node* outcome = NULL;

    if (left->process.I <= right->process.I) {
        outcome = left;
        outcome->next = merge(left->next, right);
    }
    else {
        outcome = right;
        outcome->next = merge(left, right->next);
    }

    return outcome;
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
    if (isEmpty(q) || q->ahead->next == NULL)
        return;

    mergeSort(&q->ahead);
}


/*
    A função "dequeue" remove o primeiro elemento da fila.
    Se a fila estiver vazia, exibe uma mensagem de erro.
    Caso contrário, atualiza o ponteiro "ahead" para o próximo elemento da fila.
    Se o elemento removido for o último da fila, o ponteiro "behind" também é atualizado para NULL,
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

    Node *tmp = q->ahead;
    q->ahead = q->ahead->next; // Segundo da fila

    if (q->ahead == NULL)
    {                   // Se a fila só tinha um elemento
        q->behind = NULL; // Fila se torna vazia
    }

    free(tmp); // Libera o antigo primeiro da fila
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
        printf("A fila está vazia.\n");
        return;
    }

    printf("*******************\n");

    for (Node *tmp = q->ahead; tmp != NULL; tmp = tmp->next)
    {
        printf("%s -> ", tmp->process.filename);
    }

    printf("FINAL DA FILA\n*******************\n");
}

