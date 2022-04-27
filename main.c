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
        return -1;
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

    //FLAG NA SHMM PARA TERMINAR TASK MANAGER E EDGE SERVERS
    
    // ESPERA AQUI, SÓ PODE FECHAR A SHM QUANDO TODOS OS OUTROS PROCESSOS FECHAREM
    //  WAIT FOR CHILDS
    for(int i = 0;i<3;i++){
        wait(NULL);
    }
    

    #ifdef DEBUG
    printf("aqui2\n");
    #endif

    //Semaphores
    sem_unlink("LOG_WRITE_MUTEX");
    sem_unlink("SHM_WRITE");
    sem_unlink("SHM_ES");
    sem_unlink("SHM_CHECK_PFM");
    sem_close(SMV->shm_write);
    sem_close(SMV->log_write_mutex);
    sem_close(SMV->shm_edge_servers);
    sem_close(SMV->check_performance_mode);


    pthread_cond_destroy(&SMV->edge_server_sig);
    pthread_condattr_destroy(&SMV->attr_cond);
    
    #ifdef DEBUG
    printf("aqui3\n");
    #endif

    //Pipe
    unlink(PIPE_NAME);
    close(fd_named_pipe);


    //Shared Memory
    shmdt(SMV);
    shmctl(shm_id, IPC_RMID, NULL);

    #ifdef DEBUG
    printf("aqui4\n");
    #endif

    exit(0);
}

//
//  WRITE SYNCHRONOUS TO LOG FILE AND TO THE SCREEN
//
void write_screen_log(char* str){

    FILE* flog = fopen("log.txt","a"); //trocar
    time_t now;
    struct tm timenow;

    //pthread_mutex_lock(&SMV->log_write_mutex);
    sem_wait(SMV->log_write_mutex);


    time(&now);
    localtime_r(&now,&timenow);
    fprintf(flog,"%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);
    printf("%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);

    sem_post(SMV->log_write_mutex);
    //pthread_mutex_unlock(&SMV->log_write_mutex);

    fclose(flog);

}

//
//  CLOSING SYSTEM
//
void sigint(){

    write_screen_log("Cleaning up resources");
    cleanup();
    write_screen_log("Cleanup complete! Closing system");
    exit(0);

}

//
//  SYSTEM STATS
//
void sigtstp(){
    char buffer[BUFSIZ];
    int total_tasks = 0;

    //TODO : MEAN OF RESPONSE TIME BETWEEN EACH TASK

    sem_wait(SMV->shm_write);

    for(int i = 0;i<SMV->EDGE_SERVER_NUMBER;i++){
        memset(buffer,0,BUFSIZ);
        snprintf(buffer,BUFSIZ,"Completed tasks at Edge Server %d: %d",i,edge_server_list[i].NUMBER_EXECUTED_TASKS);
        write_screen_log(buffer);

        memset(buffer,0,BUFSIZ);
        snprintf(buffer,BUFSIZ,"Completed maintenance operations at Edge Server %d: %d",i,edge_server_list[i].NUMBER_MAINTENENCE_TASKS);
        write_screen_log(buffer); 
        
        total_tasks += edge_server_list[i].NUMBER_EXECUTED_TASKS;
    }

    memset(buffer,0,BUFSIZ);
    snprintf(buffer,BUFSIZ,"Total number of completed tasks: %d",total_tasks);
    write_screen_log(buffer);

    memset(buffer,0,BUFSIZ);
    snprintf(buffer,BUFSIZ,"Number of non-executed tasks: %d",SMV->NUMBER_NON_EXECUTED_TASKS);
    write_screen_log(buffer);

}