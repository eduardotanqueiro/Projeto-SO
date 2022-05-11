//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#ifndef STD
#define STD
#include "std.h"
#endif

#include "maintenance_manager.h"
#include "monitor.h"
#include "task_manager.h"
#include <regex.h>

#define MAX_CFG_SIZE 8192

int init(char* file_name);
int check_regex(char *text, char *regex);
