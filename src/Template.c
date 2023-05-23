#include <stdio.h>
#include <unistd.h>

#define EVER ;;

int main(int argc, char* argv[]){
    
    for(EVER){
        printf("Executando %s\n",argv[0]);
        sleep(1);
    }

    return 0;
}