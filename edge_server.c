#include "edge_server.h"


int EdgeServer(int edge_server_number)
{
    pthread_t cpu_threads[2];

    #ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n",edge_server_list[edge_server_number].SERVER_NAME,edge_server_list[edge_server_number].CPU1_CAP,edge_server_list[edge_server_number].CPU2_CAP);
    #endif

    //Create vCPU's
    pthread_create(&cpu_threads[0],NULL,vCPU1,0);
    pthread_create(&cpu_threads[1],NULL,vCPU2,0);




    //Close vCPU Threads
    pthread_join(cpu_threads[0],NULL);
    pthread_join(cpu_threads[1],NULL);

    return 0;
}

void* vCPU1(){

    return 0;
}

void* vCPU2(){

    return 0;
}