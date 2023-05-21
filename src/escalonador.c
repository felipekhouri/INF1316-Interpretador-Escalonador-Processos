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

#include "data.h"

// Protótipos das funções

void handleSignal(int sig);
char* concatenateStrings(const char* str1, const char* str2);
void executeProcess(Process currentP);

// Variáveis globais

int shouldTerminate = 0;

int main(void){
    int shared_memory, shmid_pid; // IDs da memória compartilhada
    Process currentP; // Variável para armazenar o processo atual
    Process *processInfo; // Ponteiro para a informação do processo
    pid_t* pid; // Ponteiro para o ID do processo
    
    int i = 0; // Índice do processo

    struct timeval init, end; // Variáveis para medir o tempo
    float sec; // Variável para armazenar os segundos

    // Anexar memória compartilhada
    shared_memory = shmget(SHM_KEY, MAX_PROCESSOS * sizeof(Process), IPC_CREAT | 0666);
    processInfo = (Process *)shmat(shared_memory, 0, 0);
    if (!processInfo){
        perror("Erro ao anexar à memória compartilhada do processInfo.\n");
        exit(1);
    }

    // Anexar memória compartilhada
    shmid_pid = shmget(SHM_KEY2, sizeof(pid_t), IPC_CREAT | 0666);
    pid = shmat(shmid_pid, 0, 0);
    if (!pid){
        perror("Erro ao anexar à memória compartilhada do pid.\n");
        exit(1);
    }

    Queue rrQueue; // Fila para políticas Round Robin
    Queue rtQueue; // Fila para políticas Real Time
    Queue ioQueue; // Fila para políticas I/O Bound
    initQueue(&rrQueue); // Inicializa a fila Round Robin
    initQueue(&rtQueue); // Inicializa a fila Real Time
    initQueue(&ioQueue); // Inicializa a fila I/O Bound
    Process p; // Variável para armazenar o processo
    signal(SIGINT, handleSignal); // Configura o tratador de sinal para SIGINT (Ctrl+C)
    gettimeofday(&init, NULL); // Obtém o tempo de início

    while (!shouldTerminate){
        gettimeofday(&end, NULL); // Obtém o tempo atual
        sec = ((end.tv_sec - init.tv_sec) % 60); // Calcula os segundos decorridos
        printf("\n%.1f s\n", sec);
        
        if (processInfo[i].index == i){
            currentP = processInfo[i];

            if (currentP.policy == REAL_TIME){
                enqueue(&rtQueue, currentP); // Adiciona o processo na fila Real Time
                queueSort(&rtQueue); // Ordena a fila Real Time com base na prioridade
            }
            else if (currentP.policy == ROUND_ROBIN){
                enqueue(&rrQueue, currentP); // Adiciona o processo na fila Round Robin
                queueSort(&rrQueue); // Ordena a fila Round Robin com base na prioridade
            }

            i++;
        } 

        /* Inicia a execução dos processos */ 
        /* Processo do Real Time */
        if ((!isEmpty(&rtQueue)) && (rtQueue.front->process.init == sec)){
            // O primeiro da fila entra em execução
            p = rtQueue.front->process;
            if (!p.started){
                executeProcess(p); // Executa o processo pela primeira vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
                p.pid = *pid; // Pega o pid do processo
                p.started = 1; // Indica que o processo começou
            }
            else{
                kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
            }

            kill(p.pid, SIGSTOP); // Pausa o processo
            
            dequeue(&rtQueue); // Remove o processo da fila Real Time
            enqueue(&rtQueue, p); // Adiciona o processo de volta na fila Real Time
            printf("\n\nFila Real Time:\n");
            displayQueue(&rtQueue); // Imprime a Fila de processos Real Time
        }
        /* Processo do Round Robin */
        else if (!isEmpty(&rrQueue)){
            p = rrQueue.front->process;
        
            if (!p.started)
			{
                executeProcess(p); // Executa o processo pela primeira vez    
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
                p.pid = *pid; // Pega o PID do processo
                p.started = 1; // Indica que o processo começou
            }
            else
			{
                kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
                sleep(p.duration); // Deixa o programa parado pelo tempo do processo
            }
            kill(p.pid, SIGSTOP); // Pausa o processo         
            dequeue(&rrQueue); // Remove o processo da fila Round Robin
            enqueue(&rrQueue, p); // Adiciona o processo de volta na fila Round Robin
            printf("\n\nFila Round Robin:\n");
            displayQueue(&rrQueue); // Imprime a Fila de processos Round Robin
        }
    }

    /* Libera a memória compartilhada */ 
    shmctl(shared_memory, IPC_RMID, 0);
    shmctl(shmid_pid, IPC_RMID, 0);
    
    return 0;
}



/*
    A função "handleSignal" é um tratador de sinal para o sinal SIGINT.
    Quando esse sinal é recebido, a variável global "shouldTerminate" é configurada para 1,
    indicando que o programa deve terminar.
*/
void handleSignal(int sig) {
    shouldTerminate = 1;
}



/*
    A função "concatenateStrings" recebe duas strings e retorna uma nova string
    que é a concatenação das duas.
    Ela aloca memória suficiente para armazenar a nova string e a concatena
    usando as funções strcpy e strcat.
    Caso ocorra algum erro na alocação de memória, a função exibe uma mensagem de erro e encerra o programa.
*/
char* concatenateStrings(const char* str1, const char* str2) {
    size_t str1Size = strlen(str1);
    size_t str2Size = strlen(str2);
    size_t totalSize = str1Size + str2Size + 1;

    char* result = (char*)malloc(totalSize);

    if (result == NULL) {
        perror("Erro ao alocar memória");
        exit(1);
    }

    strcpy(result, str1);
    strcat(result, str2);

    return result;
}

/*
    A função "executeProcess" recebe um objeto Processo e executa o programa associado a ele.
    Ela cria um caminho completo para o programa, concatenando o diretório "./programas/" com o nome do programa.
    Em seguida, cria um array de argumentos vazio e verifica se é um novo processo (filho) usando a função fork().
    Se for um novo processo, ele substitui a imagem atual pelo programa usando execvp(),
    passando o caminho e o array de argumentos vazio.
    Após a execução do programa, a função retorna.
*/
void executeProcess(Process p){
    char *path = "./";
    char *argv[] = {p.name, NULL};
    
    path = concatenateStrings(path, p.name);
    if(fork() == 0){
        printf("Iniciando %s\n", p.name);
        execvp(path, argv);
    } 
    return;
}
