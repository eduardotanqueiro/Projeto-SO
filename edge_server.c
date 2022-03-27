#include "edge_server.h"


int EdgeServer(char *name, int CPU1_CAP, int CPU2_CAP)
{
    pthread_t cpu_threads[2];

    #ifdef DEBUG
    printf("ES_name: %s, CPU1_CAP: %d, CP2_CAP: %d\n",name,CPU1_CAP,CPU2_CAP);
    #endif

    //Create vCPU's
    pthread_create(&cpu_threads[0],NULL,vCPU1,0);
    pthread_create(&cpu_threads[1],NULL,vCPU2,0);




    //Close vCPU Threads
    pthread_join(cpu_threads[0],NULL);
    pthread_join(cpu_threads[1],NULL);

    return 0;
}

int vCPU1(){

    return 0;
}

int vCPU2(){

    return 0;
}