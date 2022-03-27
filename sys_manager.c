#include "sys_manager.h"


/*
Startup function; reads configuration file and boots system resources
*/
int init(char* file_name)
{

    //Open file
    FILE* initFile;
    if ( (initFile = fopen(file_name,"r")) == NULL)
    {
        fprintf(stderr,"Non-existent config file!!\n");
        exit(-1);
    }


    //TODO PROTEGER CONTRA MAU INPUT DO CONFIG FILE

    //Process the config file data
    int queue_pos_temp,max_wait_temp,edge_server_number_temp;

    fscanf(initFile,"%d\n%d\n%d\n",&queue_pos_temp,&max_wait_temp,&edge_server_number_temp);


    //Create the shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(Shared_Memory_Variables) + sizeof(Edge_Server)*edge_server_number_temp  , IPC_CREAT | 0700);
    if (shm_id < 1){
		perror("Error creating shm memory!\n");
		exit(1);
	}
    #ifdef DEBUG
    printf("Shared Memory created\n");
    #endif

    SMV = (Shared_Memory_Variables*) shmat(shm_id,NULL,0);
    if (SMV < (Shared_Memory_Variables*) 1){
		perror("Error attaching memory!\n");
        //cleanup(); TODO
		exit(1);
	}
    #ifdef DEBUG
    printf("Shared memory attached\n");
    #endif

    //Update some info on the shared memory
    SMV->QUEUE_POS = queue_pos_temp;
    SMV->MAX_WAIT = max_wait_temp;
    SMV->EDGE_SERVER_NUMBER = edge_server_number_temp;


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

        edge_server_list[i].IN_MAINTENANCE = 0;
        edge_server_list[i].NUMBER_EXECUTED_TASKS = 0;
        edge_server_list[i].NUMBER_MAINTENENCE_TASKS = 0;
        edge_server_list[i].PERFORMANCE_MODE = 1;
    }


  

    //Create processes
    //Monitor
    if( (SMV->child_pids[0] = fork()) == 0 )
    {
        #ifdef DEBUG
        printf("Monitor process created\n");
        #endif

        Monitor();
        exit(0);
    }
    else if ( SMV->child_pids[0] == -1)
    {
        perror("Failed to create monitor process\nClosing program...\n");
        exit(1);
    }

    //Task manager
    if( (SMV->child_pids[1] = fork()) == 0 )
    {
        #ifdef DEBUG
        printf("Task Manager process created\n");
        #endif

        TaskManager();
        exit(0);
    }
    else if ( SMV->child_pids[1] == -1)
    {
        perror("Failed to create Task Manager process\nClosing program...\n");
        exit(2);
    }

    //Maintenance Manager
    if( (SMV->child_pids[2] = fork()) == 0 )
    {
        #ifdef DEBUG
        printf("Maintenance Manager process created\n");
        #endif

        MaintenanceManager();
        exit(0);
    }
    else if ( SMV->child_pids[2] == -1)
    {
        perror("Failed to create Maintenance Manager process\nClosing program...\n");
        exit(3);
    }

    //DEBUG!!!!!
    // Wait for all worker processes
	for (int i = 0; i < 3; i++)
	{
		wait(NULL);
	}
    printf("All processes closed...\n");

    shmdt(edge_server_list);
    shmdt(SMV);
    shmctl(shm_id, IPC_RMID, NULL);
    //fim do debug

    fclose(initFile);
    return 0;
}