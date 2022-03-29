#include "task_manager.h"

int TaskManager()
{
    #ifdef DEBUG
    printf("Task Manager!!\n");
    #endif

    pid_t edge_servers_processes[SMV->EDGE_SERVER_NUMBER];

    //Start Edge Servers
    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++){

        if ( (edge_servers_processes[i] = fork()) == 0){

            EdgeServer(i);
            exit(0);
        }

    }


    //Initialize Operation Queue
    queue* operation_queue = (queue*)malloc(sizeof(queue));
    initQueue(&operation_queue);
    write_screen_log("Task Manager Operation Queue Initialized");


    //Scheduler Thread
    pthread_t tm_threads[2];
    pthread_create(&tm_threads[0],NULL,scheduler,(void*) (&operation_queue));

    //Dispatcher Thread
    pthread_create(&tm_threads[1],NULL,scheduler,(void*) (&operation_queue));




    //End Edge Servers
    for (int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++)
	{
		wait(NULL);
	}

    //Close Threads
    pthread_join(tm_threads[0],NULL);
    pthread_join(tm_threads[1],NULL);
    
    //Close queue
    deleteQueue(&operation_queue);
    free(operation_queue);
    write_screen_log("Task Manager Operation Queue Closed");

    return 0;
}

void* scheduler(void* operation_queue){

    queue** op_queue =  (queue**) operation_queue;
    #ifdef DEBUG
    printf("QUEUE SIZE TEST SCHEDULER %d\n",(*op_queue)->queue_size);
    #endif

    pthread_exit(NULL);
}

void* dispatcher(void* operation_queue){

    queue** op_queue =  (queue**) operation_queue;
    #ifdef DEBUG
    printf("QUEUE SIZE TEST DISPATCHER %d\n",(*op_queue)->queue_size);
    #endif


    pthread_exit(NULL);
}

//QUEUE
int initQueue(queue** queue){

    (*queue)->first_element = NULL;
    (*queue)->queue_size = SMV->QUEUE_POS;
    (*queue)->used_nodes = 0;
    return 0;
}

int insert(queue** queue, operation op_to_insert){

    //Allocate Memory for the queue node
    struct node *new_node = (struct node*)malloc(sizeof(struct node));

    new_node->operation = op_to_insert;
    new_node->next_node = NULL;
    new_node->priority = 0;

    if( (*queue)->first_element == NULL ){
        //The queue is empty

        new_node->previous_node = NULL;
        (*queue)->first_element = new_node;

    }
    else if( (*queue)->used_nodes == (*queue)->queue_size ){
        //The queue is full

        return 1;
    }
    else{
        //Queue has 1 or more nodes

        struct node* aux_node = (*queue)->first_element;
        
        //Search for last node
        while(aux_node->next_node != NULL){
            aux_node = aux_node->next_node;
        }

        //Insert
        new_node->previous_node = aux_node;
        aux_node->next_node = new_node;

    }


    (*queue)->used_nodes++;
    return 0;
}


int pop(queue** queue,struct node** node_to_pop){

    if( (*queue)->first_element == NULL){
        //Empty queue
        return 1;
    }
    else{

        if( (*node_to_pop)->previous_node == NULL){
            //Pop First Node
            (*queue)->first_element = (*node_to_pop)->next_node;

        }
        else if( (*node_to_pop)->next_node == NULL){
            //Pop last element
            (*node_to_pop)->previous_node->next_node = NULL;

        }

        else
        {
            //Any other possible position for the node
            (*node_to_pop)->previous_node->next_node = (*node_to_pop)->next_node;
            (*node_to_pop)->next_node->previous_node = (*node_to_pop)->previous_node;

        }

    }

    free(node_to_pop);

    return 0;
}

// cleanup queue (free memory)
//TODO REVIEW (acho que está errada/ pode ser simplificada)
int deleteQueue(queue** queue){

    while( (*queue)->first_element != NULL) {

        struct node* aux_node = (*queue)->first_element;

        while(aux_node->next_node != NULL){
            
            aux_node = aux_node->next_node;
        
        }

        free(aux_node);

    }

    return 0;
}
