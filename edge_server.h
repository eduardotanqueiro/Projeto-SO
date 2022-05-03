//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#ifndef STD
#define STD
#include "std.h"
#endif

int glob_edge_server_number;

pthread_t cpu_threads[2];


int EdgeServer(int edge_server_number);
void* vCPU(void* args);
void* MonitorEnd();
void DoMaintenance(pid_t es_pid);

typedef struct{
    int cpu;
    char task_buf[512];
}args_cpu;