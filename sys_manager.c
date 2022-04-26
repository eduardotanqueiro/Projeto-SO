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
        write_screen_log("Non-existent config file!!");
        exit(1);
    }


    //TODO PROTEGER CONTRA MAU INPUT DO CONFIG FILE

    //Process the config file data
    int queue_pos_temp,max_wait_temp,edge_server_number_temp;

    fscanf(initFile,"%d\n%d\n%d\n",&queue_pos_temp,&max_wait_temp,&edge_server_number_temp);


    //Create the shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(Shared_Memory_Variables) + sizeof(Edge_Server)*edge_server_number_temp + sizeof(int[2]) * edge_server_number_temp , IPC_CREAT | 0700);
    if (shm_id < 1){
		write_screen_log("Error creating shm memory!");
		exit(1);
	}
 
    SMV = (Shared_Memory_Variables*) shmat(shm_id,NULL,0);
    if (SMV < (Shared_Memory_Variables*) 1){
		write_screen_log("Error attaching memory!");
        //cleanup(); TODO
		exit(1);
	}


    //Create semaphores
    sem_unlink("LOG_WRITE_MUTEX");
    SMV->log_write_mutex = sem_open("LOG_WRITE_MUTEX", O_CREAT | O_EXCL,0700,1);
    sem_unlink("SHM_WRITE");
    SMV->shm_write = sem_open("SHM_WRITE", O_CREAT | O_EXCL ,0700,1);
    sem_unlink("SHM_ES");
    SMV->shm_edge_servers = sem_open("SHM_ES", O_CREAT | O_EXCL ,0700,1);
    sem_unlink("SHM_CHECK_PFM");
    SMV->shm_edge_servers = sem_open("SHM_PFM", O_CREAT | O_EXCL ,0700,1);

    pthread_cond_init(&(SMV->edge_server_sig),NULL);


    write_screen_log("Shared Memory created");
    write_screen_log("Shared Memory attached");


    //Update some info on the shared memory
    sem_wait(SMV->shm_write); //TIRAR???
    
    SMV->QUEUE_POS = queue_pos_temp;
    SMV->MAX_WAIT = max_wait_temp;
    SMV->EDGE_SERVER_NUMBER = edge_server_number_temp;
    SMV->ALL_PERFORMANCE_MODE = 1;
    SMV->NUMBER_NON_EXECUTED_TASKS = 0;


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
        edge_server_list[i].PERFORMANCE_MODE = 1;

        edge_server_list[i].AVAILABLE_CPUS[0] = 0;
        edge_server_list[i].AVAILABLE_CPUS[1] = 0;
    }

    sem_post(SMV->shm_write);




    //Create named pipe
	if ( (mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0666)<0) ) {
		write_screen_log("Cannot create pipe");
        //cleanup
		exit(1);
	}
    write_screen_log("Task piped created");


    //Create processes

    //Monitor
    if( (SMV->child_pids[0] = fork()) == 0 )
    {
        write_screen_log("Monitor process created");

        Monitor();
        exit(0);
    }
    else if ( SMV->child_pids[0] == -1)
    {
        write_screen_log("Failed to create monitor process. Closing program...");
        //cleanup
        exit(1);
    }

    //Task manager
    if( (SMV->child_pids[1] = fork()) == 0 )
    {

        write_screen_log("Task Manager process created");

        TaskManager();
        exit(0);
    }
    else if ( SMV->child_pids[1] == -1)
    {
        write_screen_log("Failed to create Task Manager process. Closing program...");
        //cleanup
        exit(2);
    }

    //Maintenance Manager
    if( (SMV->child_pids[2] = fork()) == 0 )
    {

        write_screen_log("Maintenance Manager process created");

        MaintenanceManager();
        exit(0);
    }
    else if ( SMV->child_pids[2] == -1)
    {
        write_screen_log("Failed to create Maintenance Manager process. Closing program...");
        //cleanup
        exit(3);
    }


    fclose(initFile);
    return 0;
}