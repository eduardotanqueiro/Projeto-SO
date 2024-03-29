//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

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

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <signal.h>

#include <sys/msg.h>
#include <errno.h>


// #define DEBUG //remove this line to remove debug messages
#define PIPE_NAME "TASK_PIPE"

typedef struct
{
    //No semaphore needed for access, are set once and never changed
    int EDGE_SERVER_NUMBER;
    char SERVER_NAME[12];
    int CPU1_CAP;
    int CPU2_CAP;
    int pipe[2];

    //Need semaphore for acess, because are changed by more than 1 process/thread
    int IN_MAINTENANCE;
    int AVAILABLE_CPUS[2];
    int NUMBER_EXECUTED_TASKS;
    int NUMBER_MAINTENENCE_TASKS;
} Edge_Server;

typedef struct
{
    long msgtype;
    int msg_content;

} msg;

typedef struct
{   
    //Configuration File Variables
    int QUEUE_POS; //Capacity of Task Manager Queue
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;

    //stats
    int NUMBER_NON_EXECUTED_TASKS;
    int total_response_time;

    //Semaphores
    sem_t *log_write_mutex;
    sem_t *shm_write;
    pthread_mutex_t shm_edge_servers;
    sem_t *check_performance_mode;

    pthread_condattr_t attr_cond;
    pthread_cond_t edge_server_sig;


    //Task Manager Queue
    int node_number;
    pthread_mutexattr_t attr_mutex;
    pthread_mutex_t sem_tm_queue;
    pthread_cond_t new_task_cond;

    //Maintenance Manager Message Queue
    int msqid;

    //General Edge Servers Performance Mode
    int ALL_PERFORMANCE_MODE;


    //Variables used when system is exiting
    pthread_cond_t end_system_sig;



} Shared_Memory_Variables;

//Shared Memory Variables
int shm_id;
Shared_Memory_Variables* SMV;
Edge_Server* edge_server_list;



//Functions 
void write_screen_log(char* str);
void sigint();
