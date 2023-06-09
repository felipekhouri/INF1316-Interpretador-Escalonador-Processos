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
#include "queue.h"

// Protótipos das funções

void handleSignal(int sig);
void executeProcess(Process currentP);
void processReceived(Process* processInfo, int index, Queue* roundRobinQueue, Queue* realTimeQueue, pid_t* pid);
void executeRealTimeProcess(Queue* realTimeQueue, pid_t* pid);
void executeRoundRobinProcess(Queue* roundRobinQueue, pid_t* pid);
int isRealTimeConflict(Queue* realTimeQueue, Process newProcess);

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
    shared_memory = shmget(SHM_KEY, TOTALPROCESSES * sizeof(Process), IPC_CREAT | 0666);
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

    Queue roundRobinQueue; // Fila para políticas Round Robin
    Queue realTimeQueue; // Fila para políticas Real Time
    Queue ioBoundQueue; // Fila para políticas I/O Bound
    initQueue(&roundRobinQueue); // Inicializa a fila Round Robin
    initQueue(&realTimeQueue); // Inicializa a fila Real Time
    initQueue(&ioBoundQueue); // Inicializa a fila I/O Bound
    signal(SIGINT, handleSignal); // Configura o tratador de sinal para SIGINT (Ctrl+C)
    gettimeofday(&init, NULL); // Obtém o tempo de início

    while (!shouldTerminate){
        gettimeofday(&end, NULL); // Obtém o tempo atual
        sec = ((end.tv_sec - init.tv_sec)); // Calcula os segundos decorridos
        printf("\n================================\nT: %.1fs\n", sec);
        
        if (processInfo[i].index == i){
            processReceived(processInfo, i, &roundRobinQueue, &realTimeQueue, pid);
            i++;
        } 

        /* Inicia a execução dos processos */ 
        /* Processo do Real Time */
        if ((!isQueueEmpty(&realTimeQueue)) && (realTimeQueue.ahead->process.I == (int)sec%60)){
            executeRealTimeProcess(&realTimeQueue, pid);
        }
        /* Processo do Round Robin */
        else if (!isQueueEmpty(&roundRobinQueue)){
            executeRoundRobinProcess(&roundRobinQueue, pid);
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

int isRealTimeConflict(Queue* realTimeQueue, Process newProcess){
    Node* temp = realTimeQueue->ahead;

    // Varre todos os processos na fila
    while(temp != NULL) {
        // Checa se o período de execução do novo processo se sobrepõe ao do processo atual
        if(!(newProcess.I >= temp->process.I + temp->process.D || newProcess.I + newProcess.D <= temp->process.I)) {
            return TRUE;  // Retorna 1 (verdadeiro) se há conflito
        }
        temp = temp->next;
    }
    return FALSE; // Retorna 0 (falso) se não há conflito
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
    char *argv[] = {p.filename, NULL};
    
    strcat(path, p.filename);
    if(fork() == 0){
        int shmid_pid = shmget(SHM_KEY2, sizeof(pid_t), IPC_CREAT | 0666);
        pid_t* pid = shmat(shmid_pid, 0, 0);
        *pid = getpid();
        printf("Iniciando %s | PID: %d\n", p.filename,*pid);
        execvp(path, argv);
    } 
    return;
}

/*
    Função que processa um processo recebido.
    Ela adiciona o processo na fila apropriada (Round Robin ou Real Time) com base em seu algoritmo de escalonamento.
*/
void processReceived(Process* processInfo, int index, Queue* roundRobinQueue, Queue* realTimeQueue, pid_t* pid) {
    Process currentP = processInfo[index];

    if (currentP.schedulingAlg == REAL_TIME){
        // Verifica se há conflito com os processos Real Time existentes
        if(!isRealTimeConflict(realTimeQueue, currentP)) {
            enqueue(realTimeQueue, currentP); // Adiciona o processo na fila Real Time
            queueSort(realTimeQueue); // Ordena a fila Real Time com base na prioridade
        } else {
            printf("Conflito detectado com o processo de Real Time: %s\n", currentP.filename);
        }
    }
    else if (currentP.schedulingAlg == ROUND_ROBIN){
        enqueue(roundRobinQueue, currentP); // Adiciona o processo na fila Round Robin
        queueSort(roundRobinQueue); // Ordena a fila Round Robin com base na prioridade
    }
}

/*
    Função que executa o próximo processo da fila Real Time.
    Ela inicia a execução do processo pela primeira vez ou continua sua execução se já foi iniciado.
    Após a execução do tempo do processo, ele é pausado e colocado de volta na fila.
    Em seguida, a fila é exibida.
*/
void executeRealTimeProcess(Queue* realTimeQueue, pid_t* pid) {
    Process p = realTimeQueue->ahead->process;
    if (!p.started){
        printf("|Preempção REALTIME|\n");
        executeProcess(p); // Executa o processo pela primeira vez
        sleep(p.D); // Deixa o programa parado pelo tempo do processo
        p.pid = *pid; // Pega o pid do processo
        p.started = 1; // Indica que o processo começou
    }
    else{
        printf("|Preempção REALTIME|\n");
        kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
        sleep(p.D); // Deixa o programa parado pelo tempo do processo
    }
    kill(p.pid, SIGSTOP); // Pausa o processo
    dequeue(realTimeQueue); // Remove o processo da fila Real Time
    enqueue(realTimeQueue, p); // Adiciona o processo de volta na fila Real Time
    printf("Fila REAL-TIME ");
    printQueue(realTimeQueue); // Imprime a Fila de processos Real Time
}

/*
    Função que executa o próximo processo da fila Round Robin.
    Ela inicia a execução do processo pela primeira vez ou continua sua execução se já foi iniciado.
    Após a execução do tempo do processo, ele é pausado e colocado de volta na fila.
    Em seguida, a fila é exibida.
*/
void executeRoundRobinProcess(Queue* roundRobinQueue, pid_t* pid) {
    Process p = roundRobinQueue->ahead->process;
    
    if (!p.started){
        executeProcess(p); // Executa o processo pela primeira vez    
        sleep(p.D); // Deixa o programa parado pelo tempo do processo
        p.pid = *pid; // Pega o PID do processo
        p.started = 1; // Indica que o processo começou
    }
    else{
        kill(p.pid, SIGCONT); // Continua o processo já executado uma vez
        sleep(p.D); // Deixa o programa parado pelo tempo do processo
    }
    kill(p.pid, SIGSTOP); // Pausa o processo         
    dequeue(roundRobinQueue); // Remove o processo da fila Round Robin
    enqueue(roundRobinQueue, p); // Adiciona o processo de volta na fila Round Robin
    printf("================================\nFila Round Robin ");
    printQueue(roundRobinQueue); // Imprime a Fila de processos Round Robin
}
