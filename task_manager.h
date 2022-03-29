#ifndef STD
#define STD
#include "std.h"
#endif

#include "edge_server.h"

typedef struct{
    char op_command[50];
}operation;

struct node{
    operation operation;
    struct node* next_node;
    struct node* previous_node;
    int priority;
};

typedef struct{
    struct node* first_element;
    int queue_size;
    int used_nodes;
}queue;

//Queue Functions
int initQueue(queue** queue);
int insert(queue** queue, operation op_to_insert);
int pop(queue** queue,struct node** node_to_pop);
int deleteQueue(queue** queue);

//Process
int TaskManager();
void* scheduler(void* operation_queue);
void* dispatcher(void* operation_queue);