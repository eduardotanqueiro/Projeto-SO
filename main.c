//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "mainHeader.h"

int main(int argc, char** argv){

    //Block CTRL+C and CTRL+Z during initialization
    //TODO BLOCK ALL SIGNALS, AFTER INIT RESUME ONLY CTRL+C AND CTRL+Z
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);


    if(argc == 2)
        init(argv[1]);
               
    else
    {
        fprintf(stderr,"Wrong command format\n");
        return 1;
    }

    //Redirect CTRL+C and CTRL+Z
    signal(SIGTSTP,sigtstp);
    signal(SIGINT,sigint);

    #ifdef DEBUG
    pause();
    #endif


    return 0;
}

void cleanup(){


    #ifdef DEBUG
    printf("aqui1\n");
    #endif

    //SINAL PARA AVISAR OS PROCESSOS QUE É PARA TERMINAR
    pthread_cond_broadcast(&SMV->end_system_sig);

    // ESPERA AQUI, SÓ PODE FECHAR A SHM QUANDO TODOS OS OUTROS PROCESSOS FECHAREM
    // WAIT FOR CHILDS
    for(int i = 0;i<3;i++){
        wait(NULL);
    }
    
    //PRINT ESTATÍSTICAS
    sigtstp();

    #ifdef DEBUG
    printf("aqui2\n");
    #endif

    //Maintenance Manager Message Queue
    msgctl(SMV->msqid, IPC_RMID, NULL);

    //Semaphores
    sem_unlink("SHM_WRITE");
    sem_unlink("SHM_CHECK_PFM");
    sem_close(SMV->shm_write);
    sem_close(SMV->check_performance_mode);


    pthread_cond_destroy(&SMV->edge_server_sig);
    pthread_cond_destroy(&SMV->end_system_sig);
    pthread_cond_destroy(&SMV->new_task_cond);
    pthread_condattr_destroy(&SMV->attr_cond);

    pthread_mutex_destroy(&SMV->shm_edge_servers);
    pthread_mutex_destroy(&SMV->sem_tm_queue);
    pthread_mutexattr_destroy(&SMV->attr_mutex);

}

//
//  WRITE SYNCHRONOUS TO LOG FILE AND TO THE SCREEN
//
void write_screen_log(char* str){

    FILE* flog = fopen("log.txt","a");


    time_t now;
    struct tm timenow;

    sem_wait(SMV->log_write_mutex);

    time(&now);
    localtime_r(&now,&timenow);
    fprintf(flog,"%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);
    printf("%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);

    sem_post(SMV->log_write_mutex);


    fclose(flog);

}

//
//  CLOSING SYSTEM
//
void sigint(){

    write_screen_log("CLEANING UP RESOURCES");
    cleanup();
    write_screen_log("CLEANUP COMPLETE! CLOSING SYSTEM");

    //Close log sem
    sem_unlink("LOG_WRITE_MUTEX");
    sem_close(SMV->log_write_mutex);

    //Shared Memory
    shmdt(SMV);
    shmctl(shm_id, IPC_RMID, NULL);

    //Close after, to keep the log file updated

    exit(0);

}

//
//  SYSTEM STATS
//
void sigtstp(){
    char buffer[BUFSIZ];
    int total_tasks = 0;

    pthread_mutex_lock(&SMV->shm_edge_servers);

    for(int i = 0;i<SMV->EDGE_SERVER_NUMBER;i++){

        memset(buffer,0,BUFSIZ);
        snprintf(buffer,BUFSIZ,"Completed tasks at Edge Server %d: %d",i,edge_server_list[i].NUMBER_EXECUTED_TASKS);
        write_screen_log(buffer);

        memset(buffer,0,BUFSIZ);
        snprintf(buffer,BUFSIZ,"Completed maintenance operations at Edge Server %d: %d",i,edge_server_list[i].NUMBER_MAINTENENCE_TASKS);
        write_screen_log(buffer); 
        
        total_tasks += edge_server_list[i].NUMBER_EXECUTED_TASKS;

    }

    pthread_mutex_unlock(&SMV->shm_edge_servers);

    sem_wait(SMV->shm_write);
    
    memset(buffer,0,BUFSIZ);
    if(total_tasks != 0)
        snprintf(buffer,BUFSIZ,"Mean of response time between tasks: %d", (int)SMV->total_response_time/total_tasks );
    else
        snprintf(buffer,BUFSIZ,"Mean of response time between tasks: 0");
    write_screen_log(buffer);

    memset(buffer,0,BUFSIZ);
    snprintf(buffer,BUFSIZ,"Total number of completed tasks: %d",total_tasks);
    write_screen_log(buffer);


    memset(buffer,0,BUFSIZ);
    snprintf(buffer,BUFSIZ,"Number of non-executed tasks: %d",SMV->NUMBER_NON_EXECUTED_TASKS);
    write_screen_log(buffer);
    
    sem_post(SMV->shm_write);

}



