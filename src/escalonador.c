#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include "info.h"

// Protótipos das funções
void handler(int sig);
char* concatenarStrings(const char* str1, const char* str2);
void execProcess(Process currentP);

// Variáveis globais
int termina = FALSE;

int main(void){
    int shared_memory, shmid_pid; // IDs da memória compartilhada
    Process currentP; // Variável para armazenar o processo atual
    Process *processInfo; // Ponteiro para a informação do processo
    pid_t* pid; // Ponteiro para o ID do processo
    
    int i = 0; // Índice do processo
    struct timeval init, end; // Variáveis para medir o tempo
    float sec; // Variável para armazenar os segundos

    // Anexar à memória compartilhada
    shared_memory = shmget(SHM_KEY, MAX_PROCESSOS * sizeof(Process), IPC_CREAT | 0666);
    processInfo = (Process *)shmat(shared_memory, 0, 0);
    if (!processInfo){
        perror("Erro ao anexar à memória compartilhada do processInfo.\n");
        exit(1);
    }

    // Anexar à memória compartilhada
    shmid_pid = shmget(SHM_KEY2, sizeof(pid_t), IPC_CREAT | 0666);
    pid = shmat(shmid_pid, 0, 0);
    if (!pid){
        perror("Erro ao anexar à memória compartilhada do pid.\n");
        exit(1);
    }

    Queue filaRR; // Fila para políticas Round Robin
    Queue filaRT; // Fila para políticas Real Time
    Queue filaIO; // Fila para políticas I/O Bound
    initQueue(&filaRR); // Inicializa a fila Round Robin
    initQueue(&filaRT); // Inicializa a fila Real Time
    initQueue(&filaIO); // Inicializa a fila I/O Bound
    Process p; // Variável para armazenar o processo
    signal(SIGINT, handler); // Configura o tratador de sinal para SIGINT (Ctrl+C)
    gettimeofday(&init, NULL); // Obtém o tempo de início

    while (!termina){
        gettimeofday(&end, NULL); // Obtém o tempo atual
        sec = ((end.tv_sec - init.tv_sec) % 60); // Calcula os segundos decorridos
        printf("\n%.1f'\n", sec);
        
        if (processInfo[i].index == i){
            // Se ainda recebe processo, entra aqui
            currentP = processInfo[i];

            if (currentP.policy == REAL_TIME){
                enqueue(&filaRT, currentP); // Adiciona o processo na fila Real Time
                queueSort(&filaRT); // Ordena a fila Real Time com base na prioridade
            }
            else if (currentP.policy == ROUND_ROBIN){
                enqueue(&filaRR, currentP); // Adiciona o processo na fila Round Robin
                queueSort(&filaRR); // Ordena a fila Round Robin com base na prioridade
            }

            i++;
        } 

        /* Inicia a execução dos processos */ 
        /* Processo do Real Time */
        if ((!isEmpty(&filaRT)) && (filaRT.front->process.init == sec)){
            // O primeiro da fila entra em execução
            p = filaRT.front->process;
            if (!p.started){
                execProcess(p); // Executa o processo pela primeira vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
                p.pid = *pid; // Pega o pid do processo
                p.started = TRUE; // Indica que o processo começou
            }
            else{
                kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
            }

            kill(p.pid, SIGSTOP); // Pausa o processo
            
            dequeue(&filaRT); // Remove o processo da fila Real Time
            enqueue(&filaRT, p); // Adiciona o processo de volta na fila Real Time
            printf("\n\nFila Real Time:\n");
            displayQueue(&filaRT); // Imprime a Fila de processos Real Time
        }
        /* Processo do Round Robin */
        else if (!isEmpty(&filaRR)){
            p = filaRR.front->process;
        
            if (!p.started)
			{
                execProcess(p); // Executa o processo pela primeira vez    
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
                p.pid = *pid; // Pega o PID do processo
                p.started = TRUE; // Indica que o processo começou
            }
            else
			{
                kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
            }
            kill(p.pid, SIGSTOP); // Pausa o processo         
            dequeue(&filaRR); // Remove o processo da fila Round Robin
            enqueue(&filaRR, p); // Adiciona o processo de volta na fila Round Robin
            printf("\n\nFila Round Robin:\n");
            displayQueue(&filaRR); // Imprime a Fila de processos Round Robin
        }
    }

    /* Libera a memória compartilhada */ 
    shmctl(shared_memory, IPC_RMID, 0);
    shmctl(shmid_pid, IPC_RMID, 0);
    
    return 0;
}



/*
    A função "handler" é um tratador de sinal para o sinal SIGINT (Ctrl+C).
    Quando esse sinal é recebido, a variável global "termina" é configurada para TRUE,
    indicando que o programa deve terminar.
*/
void handler(int sig) {
    termina = TRUE;
}



/*
    A função "concatenarStrings" recebe duas strings e retorna uma nova string
    que é a concatenação das duas.
    Ela aloca memória suficiente para armazenar a nova string e a concatena
    usando as funções strcpy e strcat.
    Caso ocorra algum erro na alocação de memória, a função exibe uma mensagem de erro e encerra o programa.
*/
char* concatenarStrings(const char* str1, const char* str2) {
    size_t tamanhoStr1 = strlen(str1);
    size_t tamanhoStr2 = strlen(str2);
    size_t tamanhoTotal = tamanhoStr1 + tamanhoStr2 + 1;

    char* resultado = (char*)malloc(tamanhoTotal);

    if (resultado == NULL) {
        perror("Erro ao alocar memória");
        exit(1);
    }

    strcpy(resultado, str1);
    strcat(resultado, str2);

    return resultado;
}

/*
    A função "execProcess" recebe um objeto Processo e executa o programa associado a ele.
    Ela cria um caminho completo para o programa, concatenando o diretório "./programas/" com o nome do programa.
    Em seguida, cria um array de argumentos vazio e verifica se é um novo processo (filho) usando a função fork().
    Se for um novo processo, ele substitui a imagem atual pelo programa usando execvp(),
    passando o caminho e o array de argumentos vazio.
    Após a execução do programa, a função retorna.
*/
void execProcess(Process p){
    char inicioPath[] = "./programas/";
    char *path;

    path = concatenarStrings(inicioPath, p.name);
    
    char *argv[] = {NULL};
    
    if(fork() == 0){
        printf("Iniciando o programa %s\n", path);
        execvp(path, argv);
    } 
    return;
}


