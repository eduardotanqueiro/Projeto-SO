//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#ifndef STD 
#define STD
#include "std.h"
#endif

#include "edge_server.h"

#define BUF_PIPE 512

pid_t *edge_servers_processes;
int fd_named_pipe;
pthread_t tm_threads[2];

struct Node{
    int id_node;
    int priority;
    int num_instructions;
    int timeout;
    struct tm arrive_time;
    struct Node* next_node;
};

typedef struct Node Node;

typedef struct{
    Node* first_node;
    int node_number;
} linked_list;

int id_node_counter;
linked_list *fila_mensagens;
pthread_mutex_t rd_wr_list;
pthread_cond_t new_task_cond;

//linked list
void insert_list(linked_list** lista, int priority, int num_instructions, int timeout);
int remove_from_list(linked_list** lista,int id_node);
void check_priorities(linked_list** lista);
void get_next_task(linked_list **lista, Node** next_task);
void clean_list(linked_list** lista);

//Process
int TaskManager();
void end_sig_tm();
void* scheduler();
void* dispatcher();

//
void check_cpus(Node *next_task, int **flag, int **pipe_to_send);
int try_to_send(Node *next_task);
