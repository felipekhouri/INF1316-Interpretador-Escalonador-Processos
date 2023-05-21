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
void executeProcess(Process currentP);
void processReceived(Process* processInfo, int index, Queue* rrQueue, Queue* rtQueue, pid_t* pid);
void executeRealTimeProcess(Queue* rtQueue, pid_t* pid);
void executeRoundRobinProcess(Queue* rrQueue, pid_t* pid);

// Variáveis globais
int shouldTerminate = 0;

int main(void){
    int shared_memory, shmid_pid; // IDs da memória compartilhada
    pid_t* pid; // Ponteiro para o ID do processo
    Process *processInfo; // Ponteiro para a informação do processo
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
    signal(SIGINT, handleSignal); // Configura o tratador de sinal para SIGINT (Ctrl+C)
    gettimeofday(&init, NULL); // Obtém o tempo de início

    while (!shouldTerminate){
        gettimeofday(&end, NULL); // Obtém o tempo atual
        sec = ((end.tv_sec - init.tv_sec) % 60); // Calcula os segundos decorridos
        printf("\n================================\nT: %.1fs\n", sec);
        
        if (processInfo[i].index == i){
            processReceived(processInfo, i, &rrQueue, &rtQueue, pid);
            i++;
        } 

        /* Inicia a execução dos processos */ 
        /* Processo do Real Time */
        if ((!isEmpty(&rtQueue)) && (rtQueue.front->process.init == sec)){
            executeRealTimeProcess(&rtQueue, pid);
        }
        /* Processo do Round Robin */
        else if (!isEmpty(&rrQueue)){
            executeRoundRobinProcess(&rrQueue, pid);
        }
    }

    /* Libera a memória compartilhada */ 
    shmctl(shared_memory, IPC_RMID, 0);
    shmctl(shmid_pid, IPC_RMID, 0);
    
    return 0;
}

/*
    Função que trata o sinal SIGINT (Ctrl+C).
    Quando esse sinal é recebido, a variável global "shouldTerminate" é configurada para 1,
    indicando que o programa deve terminar.
*/
void handleSignal(int sig) {
    shouldTerminate = 1;
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
    char path[20] = "./";
    char *argv[] = {p.name, NULL};
    
    strcat(path, p.name);
    if(fork() == 0){
        int shmid_pid = shmget(SHM_KEY2, sizeof(pid_t), IPC_CREAT | 0666);
        pid_t* pid = shmat(shmid_pid, 0, 0);
        *pid = getpid();
        printf("Iniciando %s | PID: %d\n", p.name,*pid);
        execvp(path, argv);
    } 
    return;
}

/*
    Função que processa um processo recebido.
    Ela adiciona o processo na fila apropriada (Round Robin ou Real Time) com base em sua política.
*/
void processReceived(Process* processInfo, int index, Queue* rrQueue, Queue* rtQueue, pid_t* pid) {
    Process currentP = processInfo[index];

    if (currentP.policy == REAL_TIME){
        enqueue(rtQueue, currentP); // Adiciona o processo na fila Real Time
        queueSort(rtQueue); // Ordena a fila Real Time com base na prioridade
    }
    else if (currentP.policy == ROUND_ROBIN){
        enqueue(rrQueue, currentP); // Adiciona o processo na fila Round Robin
        queueSort(rrQueue); // Ordena a fila Round Robin com base na prioridade
    }
}

/*
    Função que executa o próximo processo da fila Real Time.
    Ela inicia a execução do processo pela primeira vez ou continua sua execução se já foi iniciado.
    Após a execução do tempo do processo, ele é pausado e colocado de volta na fila.
    Em seguida, a fila é exibida.
*/
void executeRealTimeProcess(Queue* rtQueue, pid_t* pid) {
    Process p = rtQueue->front->process;
    if (!p.started){
        printf("Preepção REALTIME\n");
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
    dequeue(rtQueue); // Remove o processo da fila Real Time
    enqueue(rtQueue, p); // Adiciona o processo de volta na fila Real Time
    printf("Fila Real Time ");
    displayQueue(rtQueue); // Imprime a Fila de processos Real Time
}

/*
    Função que executa o próximo processo da fila Round Robin.
    Ela inicia a execução do processo pela primeira vez ou continua sua execução se já foi iniciado.
    Após a execução do tempo do processo, ele é pausado e colocado de volta na fila.
    Em seguida, a fila é exibida.
*/
void executeRoundRobinProcess(Queue* rrQueue, pid_t* pid) {
    Process p = rrQueue->front->process;
    
    if (!p.started){
        executeProcess(p); // Executa o processo pela primeira vez    
        sleep(p.duration); // Deixa o programa parado pelo tempo do processo
        p.pid = *pid; // Pega o PID do processo
        p.started = 1; // Indica que o processo começou
    }
    else{
        kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
        sleep(p.duration); // Deixa o programa parado pelo tempo do processo
    }
    kill(p.pid, SIGSTOP); // Pausa o processo         
    dequeue(rrQueue); // Remove o processo da fila Round Robin
    enqueue(rrQueue, p); // Adiciona o processo de volta na fila Round Robin
    printf("================================\nFila Round Robin ");
    displayQueue(rrQueue); // Imprime a Fila de processos Round Robin
}
