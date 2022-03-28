#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <semaphore.h> // include POSIX semaphores
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/fcntl.h>

#define DEBUG //remove this line to remove debug messages

typedef struct
{
    char SERVER_NAME[12];
    int CPU1_CAP;
    int CPU2_CAP;
    int PERFORMANCE_MODE;
    int IN_MAINTENANCE;

    int NUMBER_EXECUTED_TASKS;
    int NUMBER_MAINTENENCE_TASKS;
} Edge_Server;


typedef struct
{   
    //Configuration File Variables
    int QUEUE_POS; //Capacity of Task Manager Queue
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;

    int NUMBER_NON_EXECUTED_TASKS;

    //Processes variables
    pid_t child_pids[3]; //Task manager, Monitor and Maintenance Manager processes

    //Semaphores
    sem_t *log_write_mutex;
    sem_t *shm_write;

    //General Edge Servers Performance Mode
    int ALL_PERFORMANCE_MODE;


} Shared_Memory_Variables;

//Shared Memory Variables
int shm_id;
Shared_Memory_Variables* SMV;
Edge_Server* edge_server_list;



//Functions 
void write_screen_log(char* str);
