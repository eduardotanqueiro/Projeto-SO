#ifndef STD
#define STD
#include "std.h"
#endif

#include "edge_server.h"

pid_t *edge_servers_processes;
int fd_named_pipe;
pthread_t tm_threads[2];

typedef struct{
    long msgtype;
    char op_command[50];
} task_msg;


//Process
int TaskManager();
void end_sig_tm();
void* scheduler();
void* dispatcher();