#include "process.h"

typedef struct node
{
    Process process;
    struct node *next;
} Node;
