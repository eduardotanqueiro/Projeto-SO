#include "task_manager.h"

int TaskManager()
{
    #ifdef DEBUG
    printf("Task Manager!!\n");
    #endif

    signal(SIGINT,SIG_BLOCK);

    edge_servers_processes = malloc( sizeof(pid_t) * (SMV->EDGE_SERVER_NUMBER) );

    //Start Edge Servers
    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++){

        if ( (edge_servers_processes[i] = fork()) == 0){

            EdgeServer(i);
            exit(0);
        }

    }

    //Open Named Pipe for reading
	if ((fd_named_pipe = open(PIPE_NAME, O_RDWR)) < 0) {
		write_screen_log("Cannot open pipe");
        end_sig_tm();
		exit(1);
	}

    //TODO Create message queue

    write_screen_log("Task pipe opened");

    //Handle End Signal
    signal(SIGINT,end_sig_tm);

    //Scheduler Thread
    pthread_create(&tm_threads[0],NULL,scheduler,0);

    //Dispatcher Thread
    pthread_create(&tm_threads[1],NULL,dispatcher,0);

    #ifdef DEBUG
    pause();
    #endif


    return 0;
}

void* scheduler(){

    #ifdef DEBUG
    printf("SCHEDULER\n");
    pause();
    #endif

    pthread_exit(NULL);
}

void* dispatcher(void* operation_queue){

    #ifdef DEBUG
    printf("DISPATCHER\n");
    pause();
    #endif


    pthread_exit(NULL);
}

void end_sig_tm(){

    write_screen_log("Cleaning up Task Manager");

    
    // TODO
    // ESCREVER NO LOG AS MENSAGENS QUE RESTA NA FILA DO SCHEDULER + NAMED PIPE
    // CLOSE MESSAGE QUEUE

    //CLOSE NAMED PIPE
    unlink(PIPE_NAME);
    close(fd_named_pipe);

    //KILL EDGE SERVER PROCESSES
    for(int i = 0;i< SMV->EDGE_SERVER_NUMBER;i++){
        printf("pid edge %d\n",edge_servers_processes[i]);
        kill(edge_servers_processes[i],SIGINT);
    }


    //CLOSE THREADS
    pthread_cancel(tm_threads[0]);
    pthread_cancel(tm_threads[1]);

    // TODO
    // CLOSE UNNAMED PIPES

  
    free(edge_servers_processes);
    
    write_screen_log("Task Manager Cleanup Complete");
    exit(0);
}

