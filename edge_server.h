#ifndef STD
#define STD
#include "std.h"
#endif

pthread_t cpu_threads[2];
int end_system;
pthread_mutex_t read_end;

int EdgeServer(int edge_server_number);
void* vCPU1();
void* vCPU2();
void end_sig(); 