//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "edge_server.h"


int EdgeServer(int edge_server_number)
{


    #ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n", edge_server_list[edge_server_number].SERVER_NAME, edge_server_list[edge_server_number].CPU1_CAP, edge_server_list[edge_server_number].CPU2_CAP);
    glob_edge_server_number = edge_server_number;
    #endif

    end_system = 0;
    pthread_mutex_init(&read_end, NULL);

    //METER O CPU1 DISPONIVEL AO INICIO
    //NÃO ESTAO A SER USADOS MECANISMOS DE SINCRONIZAÇÀO AQUI PORQUE CADA EDGE SERVER VAI ALTERAR UMA ZONA DIFERENTE
    edge_server_list[ glob_edge_server_number].AVAILABLE_CPUS[0] = 1;


    // Resume CTRL+C handling on main thread
    //TODO -> TIRAR E FAZER VARIAVEL GLOBAL OU ENTAO NA SHM
    signal(SIGINT,end_sig);
 

    // TODO
    // RECEIVE TASKS AND MANAGE ACTIVE CPUS
    // UNNAMED PIPES
    char buffer[512];
    int aval_cpu1,aval_cpu2;
    args_cpu thread_args;

    while(1){

        //READ FROM PIPE FOR BUFFER
        printf("Edge Server %d waiting for messages on unnamed pipe\n",glob_edge_server_number);
        read( edge_server_list[ glob_edge_server_number].pipe[0] , buffer , 512);

        #ifdef DEBUG
        printf("DEBUG EDGE SERVER %d: %s\n",glob_edge_server_number,buffer);
        #endif

        //check performance mode
        sem_wait(SMV->check_performance_mode);
        if(SMV->ALL_PERFORMANCE_MODE == 1){

            sem_post(SMV->check_performance_mode);

            //check CPU1 available state
            sem_wait(SMV->shm_edge_servers);
            aval_cpu1 = edge_server_list[edge_server_number].AVAILABLE_CPUS[0];
            sem_post(SMV->shm_edge_servers);



            if( aval_cpu1 == 0){
                
                //esperar que o CPU fique disponivel
                pthread_join(cpu_threads[0],NULL);
            
            }

            printf("Edge Server %d before args thread\n",glob_edge_server_number);

            //send to vCPU
            thread_args.cpu = 1;
            strcpy( thread_args.task_buf, buffer);

            printf("Edge Server %d before thread\n",glob_edge_server_number);

            pthread_create(&cpu_threads[0],NULL,vCPU, (void*) &thread_args);



        }else if (SMV->ALL_PERFORMANCE_MODE == 2){
            
            sem_post(SMV->check_performance_mode);

            //prepare arguments for thread
            strcpy(thread_args.task_buf,buffer);

            //check both CPU"s available state
            sem_wait(SMV->shm_edge_servers);
            aval_cpu1 = edge_server_list[edge_server_number].AVAILABLE_CPUS[0];
            aval_cpu2 = edge_server_list[edge_server_number].AVAILABLE_CPUS[1];
            sem_post(SMV->shm_edge_servers);

            if( (aval_cpu1 == 0) && (aval_cpu2 == 0)){ //Nenhum CPU disponivel

                //TODO
                //wait for some CPU
                
            }
            else if( aval_cpu1 == 1){ //CPU1 disponível

                //send to vCPU1
                thread_args.cpu = 1;
                pthread_create(&cpu_threads[0],NULL,vCPU, (void*) &thread_args);

            }
            else if ( aval_cpu2 == 1){ //CPU2 disponível

                //send to vCPU2
                thread_args.cpu = 2;
                pthread_create(&cpu_threads[1],NULL,vCPU, (void*) &thread_args);

            }



        }




    }


    #ifdef DEBUG
    pause();
    #endif

    // end_sig();

    return 0;
}


void* vCPU(void* arg){

    args_cpu *t_args = (args_cpu*) arg; 

    //Set CPU as not available
    sem_wait(SMV->shm_edge_servers);
    edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[ t_args->cpu - 1] = 0;
    sem_post(SMV->shm_edge_servers);

    //Split task arguments with strtok
    char *tok,*resto;
    int task_id,num_instructions;

    tok = strtok_r( t_args->task_buf,";",&resto);
    task_id = atoi(tok);

    tok = strtok_r(NULL,";",&resto);
    num_instructions = atoi(tok);

    //do work
    if( t_args->cpu == 1){
        usleep( (int) (num_instructions / edge_server_list[glob_edge_server_number].CPU1_CAP  * 1000000) );
    }
    else{
        usleep( (int) (num_instructions / edge_server_list[glob_edge_server_number].CPU2_CAP  * 1000000) );
    }


    //Log
    char buf[256];
    snprintf(buf,256,"%s,CPU%d: TASK %d COMPLETED", edge_server_list[ glob_edge_server_number].SERVER_NAME, t_args->cpu, task_id );
    write_screen_log(buf);

    //Set CPU as available and update executed tasks
    sem_wait(SMV->shm_edge_servers);

    edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[ t_args->cpu - 1] = 1;
    edge_server_list[ glob_edge_server_number ].NUMBER_EXECUTED_TASKS++;

    //Avisar o task manager que há um CPU disponivel;
    pthread_cond_broadcast(&SMV->edge_server_sig);
    
    sem_post(SMV->shm_edge_servers);

    pthread_exit(NULL);
}


void end_sig()
{

    // write_screen_log("Cleaning up one edge server");
    printf("Cleaning up edge server no %d\n", glob_edge_server_number);

    // TODO
    // SIGNAL CPU THREADS TO END WORK AND FINISH
    // pthread_cancel(cpu_threads[0]);
    // pthread_cancel(cpu_threads[1]);

    pthread_mutex_lock(&read_end);
    end_system = 1;
    pthread_mutex_unlock(&read_end);

    // wait for CPU threads to end work
    for (int i = 0; i < 2; i++)
        pthread_join(cpu_threads[i], NULL);

    pthread_mutex_destroy(&read_end);

    printf("Edge server no %d ended cleanup\n", glob_edge_server_number);
    // write_screen_log("An edge server just completed cleanup");
    exit(0);
}
