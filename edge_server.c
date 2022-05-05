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
    pid_t es_pid = getpid();
    msg rcv_msg;
    rcv_msg.msgtype = glob_edge_server_number+1;
    rcv_msg.msg_content = es_pid;
    msgsnd(SMV->msqid,&rcv_msg,sizeof(rcv_msg) - sizeof(long),0);

    //Thread that controls the end of the system
    pthread_t monitor;
    pthread_create(&monitor,NULL,MonitorEnd,0);
    
    //Log
    snprintf(buf,sizeof(buf),"%s READY",edge_server_list[ glob_edge_server_number ].SERVER_NAME);
    write_screen_log(buf);

    //WORK
    char buffer[512];
    int aval_cpu1,aval_cpu2;

    args_cpu thread_args1;
    thread_args1.cpu = 1;

    args_cpu thread_args2;
    thread_args2.cpu = 2;


    while(1){

        DoMaintenance(es_pid);

        //READ FROM PIPE FOR BUFFER
        read( edge_server_list[ glob_edge_server_number].pipe[0] , buffer , 512);

        pthread_mutex_lock(&SMV->shm_edge_servers);

        // #ifdef DEBUG
        // printf("DEBUG EDGE SERVER %d: %s\n",glob_edge_server_number,buffer);
        // #endif

        //check performance mode
        sem_wait(SMV->check_performance_mode);

        if(SMV->ALL_PERFORMANCE_MODE == 1){

            sem_post(SMV->check_performance_mode);

            //send to vCPU1
            //Set CPU as not available
            edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[0] = 0;
            pthread_mutex_unlock(&SMV->shm_edge_servers);

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
                pthread_mutex_unlock(&SMV->shm_edge_servers);
                printf("EDGE SERVER %d error\n",glob_edge_server_number);
                exit(1);

            }
            else if ( aval_cpu2 == 1){ //CPU2 disponível

                //send to vCPU2
                //Set CPU as not available
                edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[1] = 0;
                pthread_mutex_unlock(&SMV->shm_edge_servers);

                //prepare arguments for thread
                strcpy(thread_args2.task_buf,buffer);

                pthread_create(&cpu_threads[1],NULL,vCPU, (void*) &thread_args2);

            }
            else if( aval_cpu1 == 1){ //CPU1 disponível TROCAR PARA O 2 PRIMEIRO

                //send to vCPU1
                //Set CPU as not available
                edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[0] = 0;
                pthread_mutex_unlock(&SMV->shm_edge_servers);

                //prepare arguments for thread
                strcpy(thread_args1.task_buf,buffer);

                pthread_create(&cpu_threads[0],NULL,vCPU, (void*) &thread_args1);


            }


            //check if any thread ended meanwhile
            // pthread_tryjoin_np(&cpu_threads[0],NULL);
            // pthread_tryjoin_np(&cpu_threads[1],NULL);

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


    //Set CPU as available and update executed tasks
    pthread_mutex_lock(&SMV->shm_edge_servers);

    //TODO check performance mode
    edge_server_list[ glob_edge_server_number ].AVAILABLE_CPUS[ t_args->cpu - 1] = 1;
    edge_server_list[ glob_edge_server_number ].NUMBER_EXECUTED_TASKS++;

    //Avisar o task manager/processo ES que há um CPU disponivel;
    pthread_cond_broadcast(&SMV->edge_server_sig);
    
    pthread_mutex_unlock(&SMV->shm_edge_servers);

    //Log
    char buf[256];
    snprintf(buf,256,"%s,CPU%d: TASK %d COMPLETED", edge_server_list[ glob_edge_server_number].SERVER_NAME, t_args->cpu, task_id );
    write_screen_log(buf);

    pthread_exit(NULL);
}



void* MonitorEnd(){

    //Wait for System Manager Signal saying that we should end servers
    //resolver isto dos mutexes
    pthread_mutex_t m; 
    struct timespec   ts; //documentação pthread_cond_timedwait() da IBM

    pthread_mutex_lock(&m);


    pthread_cond_wait(&SMV->end_system_sig,&m);

    printf("EDGE SERVER %d RECEIVED END SIGNAL\n",glob_edge_server_number);

    //check performance mode and wait for threads if necessary
    sem_wait(SMV->check_performance_mode);

    if(SMV->ALL_PERFORMANCE_MODE == 1){

        sem_post(SMV->check_performance_mode);

        pthread_mutex_lock(&SMV->shm_edge_servers);
        //Wait for CPU1 to end the work
        while( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0){

            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 3;

            //printf("%d %d %d\n",glob_edge_server_number,edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0],edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1]);
            pthread_cond_timedwait(&SMV->edge_server_sig,&SMV->shm_edge_servers,&ts);

        }



    }else if (SMV->ALL_PERFORMANCE_MODE == 2){

        sem_post(SMV->check_performance_mode);

        pthread_mutex_lock(&SMV->shm_edge_servers);
        //Wait for CPUs to end the work
        while( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0 || edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
            
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 3;
            
            //printf("%d %d %d\n",glob_edge_server_number,edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0],edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1]);
            pthread_cond_timedwait(&SMV->edge_server_sig,&SMV->shm_edge_servers,&ts);
            printf("TIMER EXPIRED ON %d, CHECKING CPUS\n",glob_edge_server_number);

        }


    }

    pthread_mutex_unlock(&SMV->shm_edge_servers);

    pthread_mutex_unlock(&m);
    pthread_mutex_destroy(&m);

    char buf[80];
    snprintf(buf,sizeof(buf),"CLOSING EDGE SERVER NO. %d", glob_edge_server_number);
    write_screen_log(buf);

    exit(0);

}

void DoMaintenance(pid_t es_pid){

    char buffer[512];
    msg rcv_msg;

    //CHECK FOR MESSAGES FROM MAINTENANCE MANAGER
        if( msgrcv(SMV->msqid,&rcv_msg, sizeof(rcv_msg) - sizeof(long),es_pid, IPC_NOWAIT) != -1){
            //received a message
            printf("received message on edge server %d, %ld %d\n",glob_edge_server_number,rcv_msg.msgtype,rcv_msg.msg_content);

            //check performance mode and wait for threads
            sem_wait(SMV->check_performance_mode);
            if(SMV->ALL_PERFORMANCE_MODE == 1){
                sem_post(SMV->check_performance_mode);

                pthread_mutex_lock(&SMV->shm_edge_servers);
                //set cpu as unavailable for maintenance
                edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;
                pthread_mutex_unlock(&SMV->shm_edge_servers);

            }
            else if( SMV->ALL_PERFORMANCE_MODE == 2){
                sem_post(SMV->check_performance_mode);

                //both CPUs working
                pthread_mutex_lock(&SMV->shm_edge_servers);
                if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0 && edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
                    
                    //Thread is working, wait for it
                    pthread_cond_wait(&SMV->edge_server_sig,&SMV->shm_edge_servers);

                    //Check which CPU ended
                    //TODO
                    //LOOP COM PTHREAD_COND_WAIT


                }
                else if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0){
                    //Only CPU1 Working
                    //Set CPU2 as unavailable
                    edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] = 0;

                    //Thread is working, wait for it
                    while(edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] == 0){
                        pthread_cond_wait(&SMV->edge_server_sig,&SMV->shm_edge_servers);
                    }

                    //Set CPU1 as unavailable
                    edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;

                }
                else if( edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
                    //Only CPU2 Working
                    //Set CPU1 as unavailable
                    edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 0;

                    //Thread is working, wait for it
                    while(edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] == 0){
                        pthread_cond_wait(&SMV->edge_server_sig,&SMV->shm_edge_servers);
                    }

                    //Set CPU2 as unavailable
                    edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] = 0;

                }

                pthread_mutex_unlock(&SMV->shm_edge_servers);

            }

            //send msg to maintenance, saying that the ES is ready to do maintenance
            rcv_msg.msgtype = es_pid + 50;
            rcv_msg.msg_content = 0;
            msgsnd(SMV->msqid,&rcv_msg,sizeof(rcv_msg) - sizeof(long),0);

            //write to log
            memset(buffer,0,sizeof(buffer));
            snprintf(buffer,sizeof(buffer),"EDGE SERVER %d GOING ON MAINTENANCE",glob_edge_server_number);
            write_screen_log(buffer);

            //wait for maitenance manager saying maintenance is done
            msgrcv(SMV->msqid,&rcv_msg, sizeof(rcv_msg) - sizeof(long),es_pid,0);

            //Check performance mode, set correct CPUs to available
            sem_wait(SMV->check_performance_mode);
            if(SMV->ALL_PERFORMANCE_MODE == 1){
                sem_post(SMV->check_performance_mode);

                pthread_mutex_lock(&SMV->shm_edge_servers);
                //set cpu as available
                edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 1;
                pthread_mutex_unlock(&SMV->shm_edge_servers);

            }
            else if( SMV->ALL_PERFORMANCE_MODE == 2){
                sem_post(SMV->check_performance_mode);

                //set both cpus as available
                pthread_mutex_lock(&SMV->shm_edge_servers);
                edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[0] = 1;
                edge_server_list[glob_edge_server_number].AVAILABLE_CPUS[1] = 1;
                pthread_mutex_unlock(&SMV->shm_edge_servers);

            }

            memset(buffer,0,sizeof(buffer));
            snprintf(buffer,sizeof(buffer),"EDGE SERVER %d ENDED MAINTENANCE",glob_edge_server_number);
            write_screen_log(buffer);

            //New CPUs available
            pthread_cond_broadcast(&SMV->edge_server_sig);

        }

}

