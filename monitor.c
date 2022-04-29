//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "monitor.h"

int Monitor()
{
    signal(SIGINT,SIG_DFL);

    #ifdef DEBUG
    printf("Monitor!!\n");
    //pause();
    #endif

    while(1){
        //recebe o sinal da thread quando é colocada uma tarefa na fila
        //pthread_cond_wait(,);

    }

    return 0;
}