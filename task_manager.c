//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "task_manager.h"

int TaskManager()
{
    #ifdef DEBUG
    printf("Task Manager!!\n");
    #endif

    // signal(SIGINT,SIG_BLOCK); //não é preciso se for feito no main?/trocar para sigaction?
    
    // Open unnamed pipes
    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++){
        pipe(edge_server_list[i].pipe);
    }


    edge_servers_processes = malloc(sizeof(pid_t) * (SMV->EDGE_SERVER_NUMBER));

    // Start Edge Servers
    for (int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++)
    {

        if ((edge_servers_processes[i] = fork()) == 0)
        {

            EdgeServer(i);
            exit(0);
        }
    }

    // Open Named Pipe for reading
    if ((fd_named_pipe = open(PIPE_NAME, O_RDWR)) < 0)
    {
        write_screen_log("Cannot open pipe");
        end_sig_tm();
        exit(1);
    }
    write_screen_log("Task pipe opened");



    // Create message queue
    id_node_counter = 0;
    fila_mensagens = (linked_list *)malloc(sizeof(linked_list));
    SMV->node_number = 0;
    fila_mensagens->first_node = NULL;

    // Handle End Signal
    signal(SIGINT, end_sig_tm);


    // Dispatcher Thread
    pthread_create(&tm_threads[1], NULL, dispatcher, 0);
    
    // Scheduler Thread
    pthread_create(&tm_threads[0], NULL, scheduler, 0);

    //condicao variavel à espera que o system acabe
    #ifdef DEBUG
    pause();
    #endif

    return 0;
}

void *scheduler()
{

#ifdef DEBUG
    printf("SCHEDULER\n");
// pause();
#endif

    char buffer_pipe[BUF_PIPE];
    int number_read;
    int id_task;
    int num_instructions;
    int timeout_priority;
    char *tok;
    char *resto;
    memset(buffer_pipe, 0, BUF_PIPE);

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd_named_pipe, &read_set);

    while (1)
    {

        // DEBUG
        /*
        for (int i = 0; i < 3; i++)
        {

            num_instructions = i;
            timeout_priority = i;

            // Reevaluate priorities and insert into message list
            pthread_mutex_lock(&rd_wr_list);
            check_priorities(&fila_mensagens);

            insert_list(&fila_mensagens, timeout_priority, num_instructions, timeout_priority);
            insert_list(&fila_mensagens, timeout_priority*2, num_instructions*2, timeout_priority*2);
            insert_list(&fila_mensagens, timeout_priority*3, num_instructions*3, timeout_priority*3);
            insert_list(&fila_mensagens, timeout_priority*5, num_instructions*5, timeout_priority*5);


            Node *aux = fila_mensagens->first_node;
            while (aux != NULL)
            {
                printf("No: %d,%d,%d\n", aux->id_node, aux->num_instructions, aux->priority);
                aux = aux->next_node;
            }

            //if(fila_mensagens->node_number == 1){
                // Avisar o dispatcher que já há mensangens na fila
                pthread_cond_signal(&new_task_cond);
            //}


            pthread_mutex_unlock(&rd_wr_list);



            printf("antes sleep\n");
            sleep(5);
            printf("depois\n");
        }
        */
        // DEBUG

        
        //Espera por mensagens no TASK_PIPE
        if( select(fd_named_pipe+1,&read_set,NULL,NULL,NULL) > 0){

            //printf("MEXEU BUFFER\n");

            if(FD_ISSET(fd_named_pipe,&read_set)){

                number_read = read(fd_named_pipe, buffer_pipe, BUF_PIPE);
                buffer_pipe[number_read] = '\0';

                if(number_read > 0){

                    printf("[DEBUG] Reading %d message from the TASK_PIPE: %s\n",number_read,buffer_pipe);

                    //TODO Check if it's a QUIT or STATS command before continuing

                    //Split arguments
                    tok = strtok_r(buffer_pipe,";",&resto);
                    id_task = atoi(tok);
                    id_task++; //podemos tirar???

                    tok = strtok_r(NULL,";",&resto);
                    num_instructions = atoi(tok);

                    tok = strtok_r(NULL,"\n",&resto);
                    timeout_priority = atoi(tok);


                    pthread_mutex_lock(&SMV->sem_tm_queue);

                    //Check if message queue is full
                    if(SMV->node_number == SMV->QUEUE_POS){
                        
                        //LOG
                        write_screen_log("TASK MANAGER QUEUE FULL, MESSAGE DISCARDED");
                        
                        sem_wait(SMV->shm_write);
                        SMV->NUMBER_NON_EXECUTED_TASKS++;
                        sem_post(SMV->shm_write);

                    }
                    else{
                        


                        //Reevaluate priorities and insert into message list
                        check_priorities(&fila_mensagens);

                        insert_list(&fila_mensagens,timeout_priority,num_instructions,timeout_priority);
                        
                        
                        //Avisa o monitor para verificar o load da fila
                        //Avisar o dispatcher que há mensangens na fila 
                        pthread_cond_broadcast(&SMV->new_task_cond);
    

                    }
                    
                    pthread_mutex_unlock(&SMV->sem_tm_queue);
                
                }
                else{
                    printf("recebeu 0\n");
                    //perror("Read: ");
                    //break;
                }

                //printf("[DEBUG] a ir pra prox msg\n");

            }

        }


        
    }

    pthread_exit(NULL);
}

void *dispatcher()
{

    #ifdef DEBUG
    printf("DISPATCHER\n");
    //pause();
    #endif

    Node *next_task = (Node*)malloc(sizeof(Node));


    while (1)
    {   

        //printf("LOCKED DISPATCHER\n");
        pthread_mutex_lock(&SMV->sem_tm_queue);
        //printf("UNLOCKED DISPATCHER %d\n",fila_mensagens->node_number);

        if (SMV->node_number == 0)
        { 
            // Se a fila está vazia, a thread espera por um sinal do scheduler a avisar que há uma nova tarefa na fila
            pthread_cond_wait(&SMV->new_task_cond, &SMV->sem_tm_queue);
        }


        // ir buscar a task com maior prioridade
        //printf("DISPATCHER DEBUG BEFORE TASK\n");
        get_next_task(&fila_mensagens, &next_task);
        //printf("DISPATCHER DEBUG AFTER TASK\n");
        

        #ifdef DEBUG
        //printf("DEBUG DISPATCHER1: id: %d, num_instrucoes %d, prioridade %d, timeout: %d\n", next_task->id_node, next_task->num_instructions, next_task->priority, next_task->timeout);
        #endif



 
        //try to send the task to some edge server. if there are none CPUs available, waits for a signal saying that some CPU just ended a task
        if( try_to_send(next_task) == 1){

            //Nenhum edge server disponivel, esperar por o sinal de algum deles e verificar novamente
            printf("DEBUG DISPATCHER: ALL EDGE SERVERS BUSY, WAITING FOR AVAILABLE CPUS\n");
            pthread_cond_wait(&SMV->edge_server_sig,&SMV->sem_tm_queue);

            printf("DEBUG DISPATCHER: 1 CPU BECAME AVAILABLE\n");
            try_to_send(next_task);

        }

        pthread_mutex_unlock(&SMV->sem_tm_queue);

    
    }



    free(next_task);
    pthread_exit(NULL);
}

int try_to_send(Node *next_task){

    int flag = 0, pipe_to_send = -1; 
    int *flag_ptr = &flag; //checking if there is any available CPU
    int *pipe_to_send_ptr = &pipe_to_send;


    //Check if CPUs are available
    check_cpus(next_task,&flag_ptr,&pipe_to_send_ptr);

    char task_str[512];
    memset(task_str,0,sizeof(task_str));

    //printf("before sending dispatcher\n");
    if( flag == 1 && pipe_to_send == -1){
        //descartada

        //mean response time / non executed tasks
        sem_wait(SMV->shm_write);
        SMV->total_response_time += time_since_arrive(next_task);
        SMV->NUMBER_NON_EXECUTED_TASKS++;
        sem_post(SMV->shm_write);

        //descartar
        snprintf(task_str,512,"TASK %d DISCARDED AT DISPATCHER: NO TIME TO COMPLETE",next_task->id_node);
        write_screen_log(task_str);
        return 0;

    }else if( flag == 1){
        //send to the correct pipe
        snprintf(task_str,sizeof(task_str),"TASK %d SELECTED FOR EXECUTION ON %s",next_task->id_node,edge_server_list[ pipe_to_send ].SERVER_NAME);
        write_screen_log(task_str);
        memset(task_str,0,sizeof(task_str));

        //mean response time
        sem_wait(SMV->shm_write);
        SMV->total_response_time += time_since_arrive(next_task);
        sem_post(SMV->shm_write);

        //send to server
        snprintf(task_str,512,"%d;%d",next_task->id_node,next_task->num_instructions);
        write(edge_server_list[ pipe_to_send ].pipe[1],&task_str,sizeof(task_str));
        return 0;

    }else
        return 1;

}

/*  
    Check for available CPUs in the Edge Servers
*/
void check_cpus(Node *next_task, int **flag, int **pipe_to_send){

    time_t now;
    struct tm check_time;

    time(&now);
    localtime_r(&now,&check_time);
    int tempo_decorrido = (abs(check_time.tm_min - next_task->arrive_time.tm_min)%60 )*60 + abs(check_time.tm_sec - next_task->arrive_time.tm_sec)%60;
    int tempo_restante = next_task->timeout - tempo_decorrido;

    sem_wait(SMV->shm_edge_servers);

    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++){ //Check if there is any available CPU and, if so, check if it has capacity to run the task in time

        if( edge_server_list[i].AVAILABLE_CPUS[0] == 1 ){ //CPU1 available on Edge server i

            if( next_task->num_instructions/edge_server_list[i].CPU1_CAP <= tempo_restante){ //CPU1 has capacity to run the task in time

                *(*pipe_to_send) = i;
                *(*flag) = 1;
                break;
            }

            *(*flag) = 1;
        }

        if ( edge_server_list[i].AVAILABLE_CPUS[1] == 1 ){ //CPU2 available on Edge server i

            if( next_task->num_instructions/edge_server_list[i].CPU2_CAP <= tempo_restante){ //CPU2 has capacity to run the task in time

                *(*pipe_to_send) = i;
                *(*flag) = 1;
                break;
            }

            *(*flag) = 1;
        }

    }

    sem_post(SMV->shm_edge_servers);


}


void end_sig_tm()
{

    write_screen_log("Cleaning up Task Manager");

    // CLOSE THREADS
    pthread_cancel(tm_threads[0]);
    pthread_cancel(tm_threads[1]);

    // TODO
    // ESCREVER NO LOG AS MENSAGENS QUE RESTA NA FILA DO SCHEDULER + NAMED PIPE
    // CLOSE MESSAGE QUEUE
    // CLEAN MESSAGEM QUEUE MEMORY!!!

    // CLOSE NAMED PIPE
    unlink(PIPE_NAME);
    close(fd_named_pipe);

    //

    // Wait for Edge Server Processes
    for (int i = 0; i < SMV->EDGE_SERVER_NUMBER; i++)
    {
        wait(NULL);
    }


    // TODO
    // CLOSE UNNAMED PIPES

    free(edge_servers_processes);

    write_screen_log("Task Manager Cleanup Complete");
    exit(0);
}

void insert_list(linked_list **lista, int priority, int num_instructions, int timeout)
{

    time_t now;

    // Create new node
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->id_node = id_node_counter++;
    new_node->priority = priority;
    new_node->num_instructions = num_instructions;
    new_node->timeout = timeout;
    new_node->next_node = NULL;

    time(&now);
    localtime_r(&now, &new_node->arrive_time);

    Node *aux_node = (*lista)->first_node;

    if (aux_node == NULL)
    {
        (*lista)->first_node = new_node;
    }
    else
    {
        // Search for insertion place
        while (aux_node->next_node != NULL)
        {
            aux_node = aux_node->next_node;
        }

        // Insert
        aux_node->next_node = new_node;
    }

    SMV->node_number++;
}

int remove_from_list(linked_list **lista, int id_node)
{

    Node *aux_node = (*lista)->first_node;

    // Verificar se é primeiro nó
    if (aux_node->id_node == id_node)
    {

        (*lista)->first_node = aux_node->next_node;
        free(aux_node);
        SMV->node_number--;
        return 0;
    }
    else
    { // Se não é o primeiro nó

        while ((aux_node->next_node != NULL) && (aux_node->next_node->id_node != id_node))
        {
            aux_node = aux_node->next_node;
        }

        // Chegámos ao ultimo node e o id não corresponde
        if (aux_node->next_node == NULL)
        {
            return 1;
        }
        else
        {
            
            Node *to_delete = aux_node->next_node;
            aux_node->next_node = aux_node->next_node->next_node;
            free(to_delete);
            SMV->node_number--;

            return 0;
        }
    }
}

void check_priorities(linked_list **lista)
{

    // time_t now;
    // struct tm check_time;
    // int elapsed_minutes;
    int elapsed_seconds;

    Node *aux_node = (*lista)->first_node;

    while (aux_node != NULL)
    {

        // Check if task has passed timeout;
        // time(&now);
        // localtime_r(&now, &check_time);

        // elapsed_minutes = abs(check_time.tm_min - aux_node->arrive_time.tm_min) % 60;
        // elapsed_seconds = elapsed_minutes * 60 + abs(check_time.tm_sec - aux_node->arrive_time.tm_sec)%60;
        elapsed_seconds = time_since_arrive(aux_node);

        if (elapsed_seconds >= aux_node->timeout)
        {

            //TODO Escrever para o log e remover a task da fila
            write_screen_log("Task x removida da fila por timeout");

            //errado, redo
            int id_to_remove = aux_node->id_node;
            aux_node = aux_node->next_node;

            remove_from_list(&fila_mensagens, id_to_remove);
        }
        else
        {

            // Ainda não passou o timeout, temos de reavaliar a prioridade da tarefa
            aux_node->priority = aux_node->priority - elapsed_seconds;

            aux_node = aux_node->next_node;
        }
    }
}

void get_next_task(linked_list **lista, Node **next_task)
{

    Node *aux_node = (*lista)->first_node;
    int id_most_priority_task = aux_node->id_node;
    int max_priority = aux_node->priority;

    (*next_task)->id_node = aux_node->id_node;
    (*next_task)->arrive_time = aux_node->arrive_time;
    (*next_task)->next_node = aux_node->next_node;
    (*next_task)->num_instructions = aux_node->num_instructions;
    (*next_task)->priority = aux_node->priority;
    (*next_task)->timeout = aux_node->timeout;

    // Get the most prioritary node
    while (aux_node != NULL)
    {

        if (aux_node->priority < max_priority)
        {

            id_most_priority_task = aux_node->id_node;
            max_priority = aux_node->priority;

            // Update the return argument with the highest priority node
            (*next_task)->id_node = aux_node->id_node;
            (*next_task)->arrive_time = aux_node->arrive_time;
            (*next_task)->next_node = aux_node->next_node;
            (*next_task)->num_instructions = aux_node->num_instructions;
            (*next_task)->priority = aux_node->priority;
            (*next_task)->timeout = aux_node->timeout;

        }

        aux_node = aux_node->next_node;
    }

    // Remove the most prioritary node from the list
    remove_from_list(lista, id_most_priority_task);
}

int time_since_arrive(Node *task){

    time_t now;
    struct tm check_time;
    int elapsed_minutes;
    int elapsed_seconds;

    time(&now);
    localtime_r(&now, &check_time);

    elapsed_minutes = abs(check_time.tm_min - task->arrive_time.tm_min) % 60;
    elapsed_seconds = elapsed_minutes * 60 + abs(check_time.tm_sec - task->arrive_time.tm_sec)%60;

    return elapsed_seconds;
}
