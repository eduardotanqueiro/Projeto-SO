//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "task_manager.h"

int TaskManager()
{
#ifdef DEBUG
    printf("Task Manager!!\n");
#endif

    // signal(SIGINT,SIG_BLOCK); //não é preciso se for feito no main?/trocar para sigaction?

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
    fila_mensagens->node_number = 0;
    fila_mensagens->first_node = NULL;

    // Semaforo para sincronizar escritura/leitura na fila de mensagens
    pthread_mutex_init(&rd_wr_list, NULL);
    pthread_cond_init(&new_task_cond, NULL);

    // Handle End Signal
    signal(SIGINT, end_sig_tm);


    // Dispatcher Thread
    pthread_create(&tm_threads[1], NULL, dispatcher, 0);
    
    // Scheduler Thread
    pthread_create(&tm_threads[0], NULL, scheduler, 0);


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

            printf("MEXEU BUFFER\n");

            if(FD_ISSET(fd_named_pipe,&read_set)){

                number_read = read(fd_named_pipe, buffer_pipe, BUF_PIPE);
                buffer_pipe[number_read] = '\0';

                if(number_read > 0){

                    printf("[DEBUG] Reading %d message from the TASK_PIPE: %s\n",number_read,buffer_pipe);

                    //Split arguments
                    tok = strtok_r(buffer_pipe,";",&resto);
                    id_task = atoi(tok);
                    id_task++; //podemos tirar???

                    tok = strtok_r(NULL,";",&resto);
                    num_instructions = atoi(tok);

                    tok = strtok_r(NULL,"\n",&resto);
                    timeout_priority = atoi(tok);

                    //Reevaluate priorities and insert into message list
                    pthread_mutex_lock(&rd_wr_list);
                    check_priorities(&fila_mensagens);

                    insert_list(&fila_mensagens,timeout_priority,num_instructions,timeout_priority);
                    
                    //Avisar o dispatcher que há mensangens na fila
                    if(fila_mensagens->node_number == 1){ //????
                        // Avisar o dispatcher que já há mensangens na fila
                        pthread_cond_signal(&new_task_cond);
                    }

                    pthread_mutex_unlock(&rd_wr_list);
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
        pthread_mutex_lock(&rd_wr_list);
        //printf("UNLOCKED DISPATCHER %d\n",fila_mensagens->node_number);

        if (fila_mensagens->node_number == 0)
        { // Se a fila está vazia, a thread espera por um sinal do scheduler a avisar que há uma nova tarefa na fila
            pthread_cond_wait(&new_task_cond, &rd_wr_list);
        }


        // ir buscar a task com maior prioridade
        //printf("DISPATCHER DEBUG BEFORE TASK\n");
        get_next_task(&fila_mensagens, &next_task);
        //printf("DISPATCHER DEBUG AFTER TASK\n");
        

        #ifdef DEBUG
        printf("DEBUG DISPATCHER1: id: %d, num_instrucoes %d, prioridade %d, timeout: %d\n", next_task->id_node, next_task->num_instructions, next_task->priority, next_task->timeout);
        #endif

        // TODO verificar para qual edge server vai ser enviado

        pthread_mutex_unlock(&rd_wr_list);
    }


    free(next_task);
    pthread_exit(NULL);
}

void end_sig_tm()
{

    write_screen_log("Cleaning up Task Manager");

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

    // CLOSE THREADS
    pthread_cancel(tm_threads[0]);
    pthread_cancel(tm_threads[1]);

    // IPCS
    pthread_mutex_destroy(&rd_wr_list);
    pthread_cond_destroy(&new_task_cond);

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

    (*lista)->node_number++;
}

int remove_from_list(linked_list **lista, int id_node)
{

    Node *aux_node = (*lista)->first_node;

    // Verificar se é primeiro nó
    if (aux_node->id_node == id_node)
    {

        (*lista)->first_node = aux_node->next_node;
        free(aux_node);
        (*lista)->node_number--;
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
            (*lista)->node_number--;

            return 0;
        }
    }
}

void check_priorities(linked_list **lista)
{

    time_t now;
    struct tm check_time;
    int elapsed_minutes;
    int elapsed_seconds;

    Node *aux_node = (*lista)->first_node;

    while (aux_node != NULL)
    {

        // Check if task has passed timeout;
        time(&now);
        localtime_r(&now, &check_time);

        elapsed_minutes = check_time.tm_min - aux_node->arrive_time.tm_min;
        elapsed_seconds = elapsed_minutes * 60 + abs(check_time.tm_sec - aux_node->arrive_time.tm_sec);

        if (elapsed_seconds >= aux_node->timeout)
        {

            // Escrever para o log e remover a task da fila
            write_screen_log("Task x removida da fila por timeout");

            aux_node = aux_node->next_node;

            remove_from_list(&fila_mensagens, aux_node->id_node);
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
