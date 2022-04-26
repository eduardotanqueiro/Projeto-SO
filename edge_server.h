//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#ifndef STD
#define STD
#include "std.h"
#endif

int glob_edge_server_number;

pthread_t cpu_threads[2];
int end_system;
pthread_mutex_t read_end;
pthread_cond_t end_cond;

int EdgeServer(int edge_server_number);
void* vCPU1();
void* vCPU2();
void* vCPU(void* args);
void end_sig();

typedef struct{
    int cpu;
    char task_buf[512];
}args_cpu;