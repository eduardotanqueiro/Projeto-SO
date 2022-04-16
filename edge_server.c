#include "edge_server.h"

int x;

int EdgeServer(int edge_server_number)
{

    signal(SIGINT,SIG_IGN);

    #ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n",edge_server_list[edge_server_number].SERVER_NAME,edge_server_list[edge_server_number].CPU1_CAP,edge_server_list[edge_server_number].CPU2_CAP);
    x = edge_server_number;
    #endif

    end_system = 0;
    pthread_mutex_init(&read_end,NULL);

    //Create vCPU's
    pthread_create(&cpu_threads[0],NULL,vCPU1,0);
    pthread_create(&cpu_threads[1],NULL,vCPU2,0);


    signal(SIGINT,end_sig);

    // TODO
    // RECEIVE TASKS AND MANAGE ACTIVE CPUS
    // UNNAMED PIPES
    #ifdef DEBUG
    pause();
    #endif  

    //end_sig();

    return 0;
}

void* vCPU1(){


    while(1){

        pthread_mutex_lock(&read_end);
        if(end_system == 1) break;
        pthread_mutex_unlock(&read_end);

        //DO WORK
        #ifdef DEBUG
        printf("CPU1 Working...\n");
        while(1);
        #endif

    }



    pthread_exit(NULL);
}

void* vCPU2(){


    while(1){

        pthread_mutex_lock(&read_end);
        if(end_system == 1) break;
        pthread_mutex_unlock(&read_end);

        //DO WORK
        #ifdef DEBUG
        printf("CPU2 Working...\n");
        while(1);
        #endif


    }


    pthread_exit(NULL);
}

void end_sig(){

    //write_screen_log("Cleaning up one edge server");
    printf("Cleaning up edge server no %d\n",x);

    // TODO
    // SIGNAL CPU THREADS TO END WORK AND FINISH
    pthread_cancel(cpu_threads[0]);
    pthread_cancel(cpu_threads[1]);

    /*
    pthread_mutex_lock(&read_end);
    end_system = 1;
    pthread_mutex_unlock(&read_end);


    wait for CPU threads to end work
    for(int i = 0;i<2;i++)
        pthread_join(cpu_threads[i],NULL);
    */

    sem_wait(SMV->shm_write);
    SMV->closed_edge_servers++;
    pthread_cond_signal(&SMV->end_cond);  
    sem_post(SMV->shm_write);  

    printf("Edge server no %d ended cleanup\n",x);
    //write_screen_log("An edge server just completed cleanup");
    exit(0);
}



/*
void end_sig(){

    //write_screen_log("Cleaning up one edge server");
    printf("Cleaning up edge server no %d\n",x);

    // TODO
    // SIGNAL CPU THREADS TO END WORK AND FINISH
    pthread_mutex_lock(&read_end);
    end_system = 1;
    pthread_mutex_unlock(&read_end);

    printf("antes do join\n");
    //wait for CPU threads to end work
    pthread_join(cpu_threads[0],NULL);
    pthread_join(cpu_threads[1],NULL);
    printf("depois do join\n");


    sem_wait(SMV->shm_write);
    SMV->closed_edge_servers++;
    pthread_cond_signal(&SMV->end_cond);  
    printf("EDGE SERVER: ended %d\n",SMV->closed_edge_servers);
    sem_post(SMV->shm_write);  

    printf("Edge server no %d ended cleanup\n",x);
    //write_screen_log("An edge server just completed cleanup");
    exit(0);
}
*/