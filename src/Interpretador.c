#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "queue.h"

// Protótipo das funções

void readProcessesFromFile(const char* filename, Process* processCollection, int* i);
void executeChildProcess(FILE* fp, Process* processCollection, int i);
void executeParentProcess();
int isRealTimeNotConflicting(Process *lp, int size, int beginning, int duration);


int main(void)
{
    int i = 0; // Índice do processo
    char filename[] = "exec.txt"; // Nome do arquivo de entrada
    size_t segmentIdentifier; // ID da memória compartilhada
    Process *processCollection; // Ponteiro para a lista de processos

    segmentIdentifier = shmget(SHM_KEY, TOTALPROCESSES * sizeof(Process), IPC_CREAT | 0666);
    if (segmentIdentifier == -1)
    {
        perror("Erro ao alocar memória compartilhada");
        exit(1);
    }
    processCollection = shmat(segmentIdentifier, 0, 0);

    FILE *fp = fopen(filename, "r"); // Abre o arquivo para leitura

    if (!fp)
    {
        puts("Erro ao abrir o arquivo.");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    { // Processo filho
        executeChildProcess(fp, processCollection, i);
    }
    else if (pid > 0)
    { // Processo pai
        executeParentProcess();
    }

    fclose(fp); // Fecha o arquivo
    return 0;
}

/*
    A função "isRealTimeNotConflicting" verifica se um novo processo pode ser executado no instante de início especificado.
    Ela percorre a lista de processos já existentes e verifica se há conflito de tempo.
    Se houver algum processo em execução no intervalo [beginning, beginning + duration] ou se o tempo de execução ultrapassar 60 segundos,
    retorna FALSE, indicando que o processo não pode ser executado.
    Caso contrário, retorna TRUE, indicando que o processo pode ser executado.
*/
int isRealTimeNotConflicting(Process *lp, int size, int beginning, int duration)
{
    for (int i = 0; i < size; i++)
    {
        int processEnd = lp[i].I + lp[i].D;

        if (beginning >= lp[i].I && beginning <= processEnd)
        {
            return FALSE;
        }

        if (beginning + duration >= lp[i].I && beginning + duration <= processEnd)
        {
            return FALSE;
        }
    }

    if (beginning + duration > 60)
    {
        return FALSE;
    }

    return TRUE;
}


/*
    A função "readProcessesFromFile" lê os processos do arquivo especificado e armazena na lista de processos.
    Ela percorre o arquivo linha por linha e, para cada linha, extrai as informações do processo.
    Se o processo for do tipo REAL TIME, verifica se pode ser executado no instante de início especificado.
    Caso seja válido, adiciona o processo na lista de processos.
*/
void readProcessesFromFile(const char* filename, Process* processCollection, int* i) {
    FILE* fp = fopen(filename, "r"); // Abre o arquivo para leitura

    if (!fp)
    {
        puts("Erro ao abrir o arquivo.");
        exit(1);
    }

    int beginning = 0; // Variável para armazenar o instante de início do processo
    int duration = 0; // Variável para armazenar a duração do processo
    char schedulingAlg; // Variável para armazenar a política do processo (I: REAL TIME, A: ROUND ROBIN)
    char processName[10]; // Nome do processo
    char line[100]; // Variável para armazenar cada linha do arquivo

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "Run %s I=%d D=%d", processName, &beginning, &duration) == 3) {
            // Se a linha corresponde ao padrão esperado (com informações de I e D),
            // define schedulingAlg como 'I'.
            schedulingAlg = 'T';
        } else if (sscanf(line, "Run %s", processName) == 1) {
            // Se a linha corresponde ao padrão esperado (sem informações adicionais),
            // define schedulingAlg como 'A' e zera beginning e duration.
            beginning = 0;
            duration = 0;
        } else {
            // Se a linha não corresponde ao padrão esperado, pula para a próxima linha.
            continue;
        }

        if (schedulingAlg == 'T')
        { // Processo REAL TIME
            if (isRealTimeNotConflicting(processCollection, *i, beginning, duration))
            {
                Process realTimeProcess;
                strcpy(realTimeProcess.filename, processName);
                realTimeProcess.index = *i;
                realTimeProcess.I = beginning;
                realTimeProcess.D = duration;
                realTimeProcess.schedulingAlg = REAL_TIME;
                realTimeProcess.started = FALSE;

                processCollection[*i] = realTimeProcess;

                (*i)++;
            }
            else
            {
                printf("Processo: (%s) inválido. Tempo de execução excede o limite permitido.\n", processName);
            }

            schedulingAlg = 'A';
            beginning = -1;
            duration = 1;
        }
        else
        { // Processo ROUND ROBIN
            Process roundRobinProcess;
            strcpy(roundRobinProcess.filename, processName);
            roundRobinProcess.index = *i;
            roundRobinProcess.I = -1;
            roundRobinProcess.D = 1;
            roundRobinProcess.schedulingAlg = ROUND_ROBIN;
            roundRobinProcess.started = FALSE;

            processCollection[*i] = roundRobinProcess;

            (*i)++;
        }
        sleep(1);
    }

    fclose(fp); // Fecha o arquivo
}


/*
    A função "executeChildProcess" executa o código do processo filho.
    Ela lê os processos do arquivo, armazena na lista de processos e realiza as operações necessárias.
*/
void executeChildProcess(FILE* fp, Process* processCollection, int i) {
    readProcessesFromFile("exec.txt", processCollection, &i);
}

/*
    A função "executeParentProcess" executa o código do processo pai.
    Ela realiza as operações do processo pai, como aguardar um tempo e executar o escalonador.
*/
void executeParentProcess() {
    sleep(1);
    char *argv[] = {NULL};
    execvp("./escalonador", argv); // Executa o escalonador
}
