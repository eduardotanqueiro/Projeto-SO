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


    //Create a object with a structure based on the initial necessary constant variables for the shared memory and attach that object to the shared memory
    shm_id_constants = shmget(IPC_PRIVATE, sizeof(SMV) , IPC_CREAT | 0700);
    #ifdef DEBUG
    printf("Initial constant memory created\n");
    #endif

    SMV = (struct Shared_Memory_Variables*) shmat(shm_id_constants,NULL,0);
    #ifdef DEBUG
    printf("Initial constant shared memory attached\n");
    #endif



    //TODO PROTEGER CONTRA MAU INPUT DO CONFIG FILE

    //Process the config file data
    fscanf(initFile,"%d\n%d\n%d\n",&SMV->QUEUE_POS,&SMV->MAX_WAIT,&SMV->EDGE_SERVER_NUMBER);

    list_edge_servers = (struct Edge_Server*) malloc( sizeof(struct Edge_Server) * SMV->EDGE_SERVER_NUMBER);


    //Put edge servers list on shared memory
    shm_id_edge_servers = shmget(IPC_PRIVATE, sizeof(list_edge_servers) , IPC_CREAT | 0700);
    #ifdef DEBUG
    printf("Edge Servers memory created\n");
    #endif

    list_edge_servers = (struct Edge_Server*) shmat(shm_id_edge_servers,NULL,0);
    #ifdef DEBUG
    printf("Edge Servers shared memory attached\n");
    #endif

    //Read properties for each Edge Server
    for(int i = 0; i < SMV->EDGE_SERVER_NUMBER ; i++){

        if(i != SMV->EDGE_SERVER_NUMBER - 1){
            fscanf(initFile,"%12[^,],%d,%d\n",&list_edge_servers[i].SERVER_NAME[0],&list_edge_servers[i].CPU1_CAP,&list_edge_servers[i].CPU2_CAP);
        }
        else
        {
            fscanf(initFile,"%12[^,],%d,%d",&list_edge_servers[i].SERVER_NAME[0],&list_edge_servers[i].CPU1_CAP,&list_edge_servers[i].CPU2_CAP);
        }


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

    shmdt(list_edge_servers);
    shmdt(SMV);
    shmctl(shm_id_constants, IPC_RMID, NULL);
    shmctl(shm_id_edge_servers, IPC_RMID, NULL);

    //free(list_edge_servers);
    //fim

    fclose(initFile);
    return 0;
}