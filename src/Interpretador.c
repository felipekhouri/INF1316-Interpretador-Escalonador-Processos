#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "data.h"

// Protótipo das funções


void executeChildProcess(FILE* fp, Process* lstProcess, int i);
void executeParentProcess();
int isProcessReady(Process *lp, int size, int beginning, int duration);
void readProcessesFromFile(const char* filename, Process* lstProcess, int* i);


int main(void)
{
    int i = 0; // Índice do processo
    char filename[] = "exec.txt"; // Nome do arquivo de entrada
    size_t thread; // ID da memória compartilhada
    Process *lstProcess; // Ponteiro para a lista de processos

    thread = shmget(SHM_KEY, MAX_PROCESSOS * sizeof(Process), IPC_CREAT | 0666);
    if (thread == -1)
    {
        perror("Erro ao alocar memória compartilhada");
        exit(1);
    }
    lstProcess = shmat(thread, 0, 0);

    FILE *fp = fopen(filename, "r"); // Abre o arquivo para leitura

    if (!fp)
    {
        puts("Erro ao abrir o arquivo.");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    { // Processo filho
        executeChildProcess(fp, lstProcess, i);
    }
    else if (pid > 0)
    { // Processo pai
        executeParentProcess();
    }

    fclose(fp); // Fecha o arquivo
    return 0;
}

/*
    A função "isProcessReady" verifica se um novo processo pode ser executado no instante de início especificado.
    Ela percorre a lista de processos já existentes e verifica se há conflito de tempo.
    Se houver algum processo em execução no intervalo [beginning, beginning + D] ou se o tempo de execução ultrapassar 60 segundos,
    retorna FALSE, indicando que o processo não pode ser executado.
    Caso contrário, retorna TRUE, indicando que o processo pode ser executado.
*/
int isProcessReady(Process *lp, int size, int beginning, int duration)
{
    for (int i = 0; i < size; i++)
    {
        int processEnd = lp[i].I + lp[i].D;
        if (beginning >= lp[i].I && beginning <= processEnd)
            return FALSE;
        
        if (beginning + duration >= lp[i].I && beginning + duration <= processEnd)
            return FALSE;
    }

    if (beginning + duration > 60)
        return FALSE;

    return TRUE;
}

/*
    A função "readProcessesFromFile" lê os processos do arquivo especificado e armazena na lista de processos.
    Ela percorre o arquivo linha por linha e, para cada linha, extrai as informações do processo.
    Se o processo for do tipo REAL TIME, verifica se pode ser executado no instante de início especificado.
    Caso seja válido, adiciona o processo na lista de processos.
*/
void readProcessesFromFile(const char* filename, Process* lstProcess, int* i) {
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
while (fscanf(fp, "%*s <%[^>]> %c=<%d> D=<%d>", processName, &schedulingAlg, &beginning, &duration) != EOF)
{
	// Processo REAL TIME
    if (schedulingAlg == 'I')
    { 
        if (isProcessReady(lstProcess, *i, beginning, duration))
        {
            Process realTimeProcess;
            strcpy(realTimeProcess.filename, processName);
            realTimeProcess.index = *i;
            realTimeProcess.I = beginning;
            realTimeProcess.D = duration;
            realTimeProcess.schedulingAlg = REAL_TIME;
            realTimeProcess.started = FALSE;

            lstProcess[*i] = realTimeProcess;

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

	 // Processo ROUND ROBIN
    else
    {
        Process roundRobinProcess;
        strcpy(roundRobinProcess.filename, processName);
        roundRobinProcess.index = *i;
        roundRobinProcess.I = -1;
        roundRobinProcess.D = 1;
        roundRobinProcess.schedulingAlg = ROUND_ROBIN;
        roundRobinProcess.started = FALSE;

        lstProcess[*i] = roundRobinProcess;

        (*i)++;
    }
    sleep(1);
}


    fclose(fp); 
}

/*
    A função "executeChildProcess" executa o código do processo filho.
    Ela lê os processos do arquivo, armazena na lista de processos e realiza as operações necessárias.
*/
void executeChildProcess(FILE* fp, Process* lstProcess, int i) {
    readProcessesFromFile("exec.txt", lstProcess, &i);
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
