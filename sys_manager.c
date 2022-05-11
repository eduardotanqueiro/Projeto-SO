//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028


#include "sys_manager.h"

//
//  Startup function; reads configuration file and boots system resources
//
int init(char* file_name)
{

    //Open file
    FILE* initFile;
    if ( (initFile = fopen(file_name,"r")) == NULL)
    {
        printf("Non-existent config file!!\n");
        exit(1);
    }

    //Open Log File
    //flog = fopen("log.txt","a");

    //Check file with regex functions
    char *all_cfg_file = (char*) malloc(sizeof(char) * MAX_CFG_SIZE);

    if ( fread(all_cfg_file,1,MAX_CFG_SIZE,initFile) > MAX_CFG_SIZE ){
        printf("ERROR READING CONFIG FILE\n");
        exit(1);
    }

    if( check_regex(all_cfg_file, "^([0-9]+(\r\n|\r|\n)){3}([a-zA-Z0-9_]{3,},[0-9]+,[0-9]+(\r\n|\r|\n)?){2,}$") != 0){
        printf("INVALID CONFIG FILE FORMAT\n");
        exit(1);
    }

    free(all_cfg_file);
    rewind(initFile);


    //Process the config file data
    int queue_pos_temp,max_wait_temp,edge_server_number_temp;

    fscanf(initFile,"%d\n%d\n%d\n",&queue_pos_temp,&max_wait_temp,&edge_server_number_temp);


    //Create the shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(Shared_Memory_Variables) + sizeof(Edge_Server)*edge_server_number_temp, IPC_CREAT | 0700);
    if (shm_id < 1){
		write_screen_log("Error creating shm memory!");
		exit(1);
	}
 
    SMV = (Shared_Memory_Variables*) shmat(shm_id,NULL,0);
    if (SMV < (Shared_Memory_Variables*) 1){
		write_screen_log("Error attaching memory!");
		exit(1);
	}


    //Create semaphores
    sem_unlink("LOG_WRITE_MUTEX");
    SMV->log_write_mutex = sem_open("LOG_WRITE_MUTEX", O_CREAT | O_EXCL,0700,1);
    sem_unlink("SHM_WRITE");
    SMV->shm_write = sem_open("SHM_WRITE", O_CREAT | O_EXCL ,0700,1);
    sem_unlink("SHM_CHECK_PFM");
    SMV->check_performance_mode = sem_open("SHM_CHECK_PFM", O_CREAT | O_EXCL ,0700,1);


    pthread_condattr_init(&SMV->attr_cond);
    pthread_condattr_setpshared(&SMV->attr_cond,PTHREAD_PROCESS_SHARED);

    pthread_cond_init(&SMV->edge_server_sig, &SMV->attr_cond);
    pthread_cond_init(&SMV->end_system_sig,&SMV->attr_cond);

    pthread_cond_init(&SMV->new_task_cond,&SMV->attr_cond);

    pthread_cond_init(&SMV->edge_server_move,&SMV->attr_cond);

    pthread_mutexattr_init(&SMV->attr_mutex);
    pthread_mutexattr_setpshared(&SMV->attr_mutex,PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&SMV->shm_edge_servers,&SMV->attr_mutex);
    pthread_mutex_init(&SMV->sem_tm_queue,&SMV->attr_mutex);


    write_screen_log("SHARED MEMORY CREATED");


    //Update some info on the shared memory
    sem_wait(SMV->shm_write); //TIRAR???
    
    SMV->QUEUE_POS = queue_pos_temp;
    SMV->MAX_WAIT = max_wait_temp;
    SMV->EDGE_SERVER_NUMBER = edge_server_number_temp;
    SMV->ALL_PERFORMANCE_MODE = 1;
    SMV->NUMBER_NON_EXECUTED_TASKS = 0;

    SMV->total_response_time = 0;


    //Put edge servers on shared memory
    //Read properties for each Edge Server
    edge_server_list = (Edge_Server*) (SMV + 1);
    
    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER ; i++){

        if(i != SMV->EDGE_SERVER_NUMBER - 1){
            fscanf(initFile,"%12[^,],%d,%d\n",&edge_server_list[i].SERVER_NAME[0],&edge_server_list[i].CPU1_CAP,&edge_server_list[i].CPU2_CAP);
        }
        else
        {
            fscanf(initFile,"%12[^,],%d,%d",&edge_server_list[i].SERVER_NAME[0],&edge_server_list[i].CPU1_CAP,&edge_server_list[i].CPU2_CAP);
        }

        //TODO CHECKAR SE O CPU1_CAP > CPU2_CAP, SE SIM FECHAR PROGRAMA????

        edge_server_list[i].EDGE_SERVER_NUMBER = i;
        edge_server_list[i].IN_MAINTENANCE = 0;
        edge_server_list[i].NUMBER_EXECUTED_TASKS = 0;
        edge_server_list[i].NUMBER_MAINTENENCE_TASKS = 0;

        edge_server_list[i].AVAILABLE_CPUS[0] = 0;
        edge_server_list[i].AVAILABLE_CPUS[1] = 0;
    }

    sem_post(SMV->shm_write);




    //Create named pipe
	if ( (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0666)<0) ) {
		write_screen_log("Cannot create pipe");
        sigint();
		exit(1);
	}
    write_screen_log("TASK PIPE CREATED");

    //Create Message Queue
    if( (SMV->msqid = msgget( ftok("./TASK_PIPE",1) , IPC_CREAT|0700) ) == -1){
        write_screen_log("CANNOT CREATE MAINTENANCE MANAGER MESSAGE QUEUE");
        exit(1);
    }    


    //Create processes

    //Monitor
    if( (SMV->child_pids[0] = fork()) == 0 )
    {
        write_screen_log("MONITOR PROCESS CREATED");

        Monitor();
        exit(0);
    }
    else if ( SMV->child_pids[0] == -1)
    {
        write_screen_log("Failed to create monitor process. Closing program...");
        sigint();
        exit(1);
    }

    //Task manager
    if( (SMV->child_pids[1] = fork()) == 0 )
    {

        write_screen_log("TASK MANAGER PROCESS CREATED");

        TaskManager();
        exit(0);
    }
    else if ( SMV->child_pids[1] == -1)
    {
        write_screen_log("Failed to create Task Manager process. Closing program...");
        sigint();
        exit(2);
    }

    //Maintenance Manager
    if( (SMV->child_pids[2] = fork()) == 0 )
    {

        write_screen_log("MAINTENANCE MANAGER PROCESS CREATED");

        MaintenanceManager();
        exit(0);
    }
    else if ( SMV->child_pids[2] == -1)
    {
        write_screen_log("Failed to create Maintenance Manager process. Closing program...");
        sigint();
        exit(3);
    }


    fclose(initFile);
    return 0;
}


int check_regex(char *text, char *regex){

    regex_t reg;

    regcomp(&reg,regex,REG_EXTENDED);

    int rt = regexec(&reg,text,0,NULL,0);

    regfree(&reg);

    return rt;
}