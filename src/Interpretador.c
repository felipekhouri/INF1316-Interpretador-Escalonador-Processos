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
int isOK(Process *lp, int tam, int inicio, int duracao);
void readProcessesFromFile(const char* filename, Process* lstProcess, int* i);
void executeChildProcess(FILE* fp, Process* lstProcess, int i);
void executeParentProcess();

int main(void)
{
    int i = 0; // Índice do processo
    char filename[] = "exec.txt"; // Nome do arquivo de entrada
    size_t segmento; // ID da memória compartilhada
    Process *lstProcess; // Ponteiro para a lista de processos

    segmento = shmget(SHM_KEY, MAX_PROCESSOS * sizeof(Process), IPC_CREAT | 0666);
    if (segmento == -1)
    {
        perror("Erro ao alocar memória compartilhada");
        exit(1);
    }
    lstProcess = shmat(segmento, 0, 0);

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
    A função "isOK" verifica se um novo processo pode ser executado no instante de início especificado.
    Ela percorre a lista de processos já existentes e verifica se há conflito de tempo.
    Se houver algum processo em execução no intervalo [inicio, inicio + duracao] ou se o tempo de execução ultrapassar 60 segundos,
    retorna FALSE, indicando que o processo não pode ser executado.
    Caso contrário, retorna TRUE, indicando que o processo pode ser executado.
*/
int isOK(Process *lp, int tam, int inicio, int duracao)
{
    for (int i = 0; i < tam; i++)
    {
        if ((inicio >= lp[i].init) && (inicio <= (lp[i].init + lp[i].duration)))
        {
            return FALSE;
        }

        if (((inicio + duracao) >= lp[i].init) && ((inicio + duracao) <= (lp[i].init + lp[i].duration)))
        {
            return FALSE;
        }
    }

    if ((inicio + duracao) > 60)
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
void readProcessesFromFile(const char* filename, Process* lstProcess, int* i) {
    FILE* fp = fopen(filename, "r"); // Abre o arquivo para leitura

    if (!fp)
    {
        puts("Erro ao abrir o arquivo.");
        exit(1);
    }

    int inicio = 0; // Variável para armazenar o instante de início do processo
    int duracao = 0; // Variável para armazenar a duração do processo
    char policy; // Variável para armazenar a política do processo (I: REAL TIME, A: ROUND ROBIN)
    char processName[10]; // Nome do processo

    while (fscanf(fp, "%*s <%[^>]> %c=<%d> D=<%d>", processName, &policy, &inicio, &duracao) != EOF)
    {
        if (policy == 'I')
        { // Processo REAL TIME
            if (isOK(lstProcess, *i, inicio, duracao))
            {
                strcpy(lstProcess[*i].name, processName);
                lstProcess[*i].index = *i;
                lstProcess[*i].init = inicio;
                lstProcess[*i].duration = duracao;
                lstProcess[*i].policy = REAL_TIME;
                lstProcess[*i].started = FALSE;

                (*i)++;
            }
            else
            {
                printf("Processo: (%s) inválido. Tempo de execução excede o limite permitido.\n", processName);
            }

            policy = 'A';
            inicio = -1;
            duracao = 1;
        }
        else
        { // Processo ROUND ROBIN
            strcpy(lstProcess[*i].name, processName);
            lstProcess[*i].index = *i;
            lstProcess[*i].init = -1;
            lstProcess[*i].duration = 1;
            lstProcess[*i].policy = ROUND_ROBIN;
            lstProcess[*i].started = FALSE;
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
