//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "monitor.h"

int Monitor()
{
    signal(SIGINT,SIG_DFL);

    #ifdef DEBUG
    printf("Monitor!!\nTESTING PERFORMANCE MODE 2\n");

    sem_wait(SMV->check_performance_mode);
    SMV->ALL_PERFORMANCE_MODE = 2;

    sem_wait(SMV->shm_edge_servers);
    for(int i = 0;i< SMV->EDGE_SERVER_NUMBER; i++){
        edge_server_list[i].AVAILABLE_CPUS[1] = 1;
    }
    sem_post(SMV->shm_edge_servers);



    sem_post(SMV->check_performance_mode);

    pause();
    #endif

    while(1){
        //recebe o sinal da thread quando é colocada uma tarefa na fila
        //pthread_cond_wait(,);

        if(  > 0.8){ //se a fila está ocupada a >80%

            sem_wait(SMV->check_performance_mode);
            SMV->ALL_PERFORMANCE_MODE = 2;

            sem_wait(SMV->shm_edge_servers);
            for(int i = 0;i< SMV->EDGE_SERVER_NUMBER; i++){
                edge_server_list[i].AVAILABLE_CPUS[1] = 1;
            }
            sem_post(SMV->shm_edge_servers);



            sem_post(SMV->check_performance_mode);

        }

        sem_wait(SMV->check_performance_mode);

        if( (SMV->ALL_PERFORMANCE_MODE == 2) && ( < 0.2) ){ //caiu para 20% ocupação

            SMV->ALL_PERFORMANCE_MODE = 1;

            sem_wait(SMV->shm_edge_servers);
            for(int i = 0;i< SMV->EDGE_SERVER_NUMBER; i++){
                edge_server_list[i].AVAILABLE_CPUS[1] = 0;
            }
            sem_post(SMV->shm_edge_servers);
        }

        sem_post(SMV->check_performance_mode);

    }

    return 0;
}