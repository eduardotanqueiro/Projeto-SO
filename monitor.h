//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#ifndef STD
#define STD
#include "std.h"
#endif

pthread_t monitor_end;

int Monitor();
void *MonitorWork();
void thread_cleanup_handler_monitor(void* arg);