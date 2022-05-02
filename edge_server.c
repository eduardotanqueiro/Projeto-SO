//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "edge_server.h"


int EdgeServer(int edge_server_number)
{
    char buf[100];

    #ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n", edge_server_list[edge_server_number].SERVER_NAME, edge_server_list[edge_server_number].CPU1_CAP, edge_server_list[edge_server_number].CPU2_CAP);
    glob_edge_server_number = edge_server_number;
    #endif


    //METER O CPU1 DISPONIVEL AO INICIO
    //NÃO ESTAO A SER USADOS MECANISMOS DE SINCRONIZAÇÃO AQUI PORQUE CADA EDGE SERVER VAI ALTERAR UMA ZONA DIFERENTE
    edge_server_list[ glob_edge_server_number].AVAILABLE_CPUS[0] = 1;



    //Avisar Maintenance que está a trabalhar
    //put message on MQ

    //Thread that controls the end of the system
    pthread_t monitor;
    pthread_create(&monitor,NULL,MonitorEnd,0);
    
    //Log
    snprintf(buf,sizeof(buf),"%s READY",edge_server_list[ glob_edge_server_number ].SERVER_NAME);
    write_screen_log(buf);

    //WORK
    char buffer[512];
    int aval_cpu1,aval_cpu2;

    msg rcv_msg;

    args_cpu thread_args1;
    thread_args1.cpu = 1;

    args_cpu thread_args2;
    thread_args2.cpu = 2;

    while(1){

        //CHECK FOR MESSAGES FROM MAINTENANCE MANAGER
        if( msgrcv(SMV->msqid,&rcv_msg, sizeof(rcv_msg) - sizeof(long),glob_edge_server_number, IPC_NOWAIT) != -1){
            //received a message

            if(rcv_msg.msg_flag == 0){

                //check performance mode and wait for threads
                sem_wait(SMV->check_performance_mode);
                if(SMV->ALL_PERFORMANCE_MODE == 1){
                    sem_post(SMV->check_performance_mode);

                    pthread_mutex_lock(SMV->shm_edge_servers);
                    //set cpu as unavailable for maintenance
                    edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;
                    pthread_mutex_unlock(SMV->shm_edge_servers);

                }
                else if( SMV->ALL_PERFORMANCE_MODE == 2){
                    sem_post(SMV->check_performance_mode);

                    //both CPUs working
                    pthread_mutex_lock(SMV->shm_edge_servers);
                    if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0 && edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
                        
                        //Thread is working, wait for it
                        pthread_cond_wait(SMV->edge_server_sig,SMV->shm_edge_servers);

                        //Check which CPU ended


                    }
                    else if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0){
                        //Only CPU1 Working
                        //Set CPU2 as unavailable
                        edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] = 0;

                        //Thread is working, wait for it
                        pthread_cond_wait(SMV->edge_server_sig,SMV->shm_edge_servers);

                        //Set CPU1 as unavailable
                        edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;

                    }
                    else if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
                        //Only CPU2 Working
                        //Set CPU1 as unavailable
                        edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;

                        //Thread is working, wait for it
                        pthread_cond_wait(SMV->edge_server_sig,SMV->shm_edge_servers);

                        //Set CPU2 as unavailable
                        edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] = 0;

                    }

                    pthread_mutex_unlock(SMV->shm_edge_servers);

                }

                //send msg to maintenance, saying that the ES is ready to do maintenance
                rcv_msg.msg_flag = 1;
                msgsnd(SMV->msqid,&rcv_msg,sizeof(rcv_msg) - sizeof(long),0);

                //write to log
                memset(buffer,0,sizeof(buffer));
                snprintf(buffer,sizeof(buffer),"EDGE SERVER %d GOING ON MAITENANCE",glob_edge_server_number);
                write_screen_log(buffer);

                //wait for maitenance manager saying maintenance is done
                msgrcv(SMV->msqid,&rcv_msg, sizeof(rcv_msg) - sizeof(long),glob_edge_server_number,0);

                //Check performance mode, set correct CPUs to available

                if(rcv_msg.msg_flag != 2){
                    printf("[DEBUG] some kind of error on message queue from maintenance maanger\n");
                }

            }
            else{
                printf("[DEBUG] some kind of error on message queue from maintenance maanger\n");
            }

        }

        //READ FROM PIPE FOR BUFFER
        //printf("Edge Server %d waiting for messages on unnamed pipe\n",glob_edge_server_number);
        read( edge_server_list[ glob_edge_server_number].pipe[0] , buffer , 512);

        // #ifdef DEBUG
        // printf("DEBUG EDGE SERVER %d: %s\n",glob_edge_server_number,buffer);
        // #endif
        pthread_mutex_lock(SMV->shm_edge_servers);

        //check performance mode
        sem_wait(SMV->check_performance_mode);

        if(SMV->ALL_PERFORMANCE_MODE == 1){

            sem_post(SMV->check_performance_mode);

            //check CPU1 available state

            // aval_cpu1 = edge_server_list[edge_server_number].AVAILABLE_CPUS[0];

            
            // if( aval_cpu1 == 0){// case of error in some way
                
            //     //esperar que o CPU fique disponivel
            //     pthread_join(cpu_threads[0],NULL);
            
            // }

            //send to vCPU1

            //Set CPU as not available
            edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[0] = 0;
            pthread_mutex_unlock(SMV->shm_edge_servers);

            //Argument for thread
            strcpy( thread_args1.task_buf, buffer);

            pthread_create(&cpu_threads[0],NULL,vCPU, (void*) &thread_args1);
            
            //Wait for thread end
            pthread_join(cpu_threads[0],NULL);


        }else if (SMV->ALL_PERFORMANCE_MODE == 2){
            
            sem_post(SMV->check_performance_mode);

            //check both CPU"s available state
            aval_cpu1 = edge_server_list[edge_server_number].AVAILABLE_CPUS[0];
            aval_cpu2 = edge_server_list[edge_server_number].AVAILABLE_CPUS[1];


            if( (aval_cpu1 == 0) && (aval_cpu2 == 0)){ //Nenhum CPU disponivel

                //error? shouldnt come here, só vêm tarefas pro server quando há algum cpu disponivel
                pthread_mutex_unlock(SMV->shm_edge_servers);
                printf("EDGE SERVER %d error\n",glob_edge_server_number);
                exit(1);

            }
            else if( aval_cpu1 == 1){ //CPU1 disponível TROCAR PARA O 2 PRIMEIRO

                //send to vCPU1

                //prepare arguments for thread
                strcpy(thread_args1.task_buf,buffer);

                //Set CPU as not available
                edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[0] = 0;
                pthread_mutex_unlock(SMV->shm_edge_servers);

                pthread_create(&cpu_threads[0],NULL,vCPU, (void*) &thread_args1);


            }
            else if ( aval_cpu2 == 1){ //CPU2 disponível

                //send to vCPU2
                
                //prepare arguments for thread
                strcpy(thread_args2.task_buf,buffer);

                //Set CPU as not available
                edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[1] = 0;
                pthread_mutex_unlock(SMV->shm_edge_servers);

                pthread_create(&cpu_threads[1],NULL,vCPU, (void*) &thread_args2);

            }

            //check if any thread ended (NOHANG PTHREAD JOIN)



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
    pthread_mutex_lock(SMV->shm_edge_servers);

    //TODO check performance mode
    edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[ t_args->cpu - 1] = 1;
    edge_server_list[ glob_edge_server_number ].NUMBER_EXECUTED_TASKS++;

    //Avisar o task manager/processo ES que há um CPU disponivel;
    pthread_cond_broadcast(&SMV->edge_server_sig);
    
    pthread_mutex_unlock(SMV->shm_edge_servers);

    pthread_exit(NULL);
}



void* MonitorEnd(){

    //Wait for System Manager Signal saying that we should end servers
    //resolver isto dos mutexes
    //trocar para sinais tipo SIGUSR1?? Verificar unblocking the signals em apenas 1 thread
    pthread_mutex_t m;
    pthread_mutex_lock(&m);

    pthread_cond_wait(&SMV->end_system_sig,&m);

    //printf("EDGE SERVER %d RECEIVED END SIGNAL\n",glob_edge_server_number);

    //check performance mode and wait for threads if necessary
    sem_wait(SMV->check_performance_mode);

    if(SMV->ALL_PERFORMANCE_MODE == 1){

        sem_post(SMV->check_performance_mode);

        //check CPU1 available state
        pthread_mutex_lock(SMV->shm_edge_servers);
        int aval_cpu1 = edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0];
        pthread_mutex_unlock(SMV->shm_edge_servers);

        if( aval_cpu1 == 0){
            
            //esperar que o CPU termine
            pthread_join(cpu_threads[0],NULL);
        
        }


    }else if (SMV->ALL_PERFORMANCE_MODE == 2){
        
        sem_post(SMV->check_performance_mode);


        //check both CPUs available state
        pthread_mutex_lock(SMV->shm_edge_servers);
        int aval_cpu1 = edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0];
        int aval_cpu2 = edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1];
        pthread_mutex_unlock(SMV->shm_edge_servers);

        if( (aval_cpu1 == 0) && (aval_cpu2 == 0)){ //Nenhum CPU disponivel

            //wait for both CPUs
            pthread_join(cpu_threads[0],NULL);
            pthread_join(cpu_threads[1],NULL);

        }
        else if( aval_cpu2 == 0){ //CPU1 disponível

            //wait for CPU2
            pthread_join(cpu_threads[1],NULL);

        }
        else if ( aval_cpu1 == 0 ){ //CPU2 disponível

            //wait for CPU1
            pthread_join(cpu_threads[0],NULL);

        }



    }

    char buf[80];
    snprintf(buf,sizeof(buf),"CLOSING EDGE SERVER NO. %d", glob_edge_server_number);
    write_screen_log(buf);

    exit(0);

}

