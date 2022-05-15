//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "monitor.h"

int Monitor()
{
    
    pthread_t monitor_work_thread;
    pthread_create(&monitor_work_thread,NULL,MonitorWork,NULL);
    pthread_detach(monitor_work_thread);

    //Wait for end signal
    pthread_mutex_t aux_m = PTHREAD_MUTEX_INITIALIZER; 
    pthread_mutex_lock(&aux_m);

    pthread_cond_wait(&SMV->end_system_sig,&aux_m);

    pthread_cancel(monitor_work_thread);

    pthread_mutex_unlock(&aux_m);
    pthread_mutex_destroy(&aux_m);

    exit(0);
}

void* MonitorWork(){


    while(1){
        //recebe o sinal da thread quando é colocada uma tarefa na fila
        pthread_mutex_lock(&SMV->sem_tm_queue);

        pthread_cond_wait(&SMV->new_task_cond,&SMV->sem_tm_queue);


        sem_wait(SMV->check_performance_mode);

        if((SMV->ALL_PERFORMANCE_MODE == 1) && ((double)SMV->node_number/ (double)SMV->QUEUE_POS > 0.8)){ //se a fila está ocupada a >80%

            write_screen_log("QUEUE ALMOST FULL: CHANGING PERFORMANCE MODE TO 2");

            SMV->ALL_PERFORMANCE_MODE = 2;

            pthread_mutex_lock(&SMV->shm_edge_servers);
            for(int i = 0;i< SMV->EDGE_SERVER_NUMBER; i++){
                edge_server_list[i].AVAILABLE_CPUS[1] = 1;
            }
            pthread_mutex_unlock(&SMV->shm_edge_servers);
        }


        if( (SMV->ALL_PERFORMANCE_MODE == 2) && ( (double)SMV->node_number/ (double)SMV->QUEUE_POS < 0.2) ){ //caiu para 20% ocupação

            write_screen_log("CHANGING PERFORMANCE MODE TO 1: POWER SAVING");

            SMV->ALL_PERFORMANCE_MODE = 1;

            pthread_mutex_lock(&SMV->shm_edge_servers);
            for(int i = 0;i< SMV->EDGE_SERVER_NUMBER; i++){
                edge_server_list[i].AVAILABLE_CPUS[1] = 0;
            }
            pthread_mutex_unlock(&SMV->shm_edge_servers);
        }

        sem_post(SMV->check_performance_mode);

        pthread_mutex_unlock(&SMV->sem_tm_queue);

    }

    pthread_exit(NULL);
}

