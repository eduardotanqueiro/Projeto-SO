#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <semaphore.h> // include POSIX semaphores
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#define DEBUG //remove this line to remove debug messages

struct Edge_Server
{
    char SERVER_NAME[12];
    int CPU1_CAP;
    int CPU2_CAP;
    int PERFORMANCE_MODE;
    int IN_MAINTENANCE;

    int NUMBER_EXECUTED_TASKS;
    int NUMBER_MAINTENENCE_TASKS;
};


struct Shared_Memory_Variables
{   
    //Configuration File Variables
    int QUEUE_POS;
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;

    int NUMBER_NON_EXECUTED_TASKS;

    //Processes variables
    pid_t child_pids[3]; //Task manager, Monitor and Maintenance Manager




    int ALL_PERFORMANCE_MODE;
};

int shm_id_constants;
struct Shared_Memory_Variables* SMV;

int shm_id_edge_servers;
struct Edge_Server* list_edge_servers;