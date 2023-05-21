#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/time.h>

#define EVER ;;
#define SHM_KEY2 7000

int main(int argc, char* argv[]){
    
    for(EVER){
        printf("Executando %s\n",argv[0]);
        sleep(1);
    }

    return 0;
}