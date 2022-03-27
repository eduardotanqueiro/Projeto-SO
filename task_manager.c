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




    //End Edge Servers
    for (int i = 0; i < 3; i++)
	{
		wait(NULL);
	}


    return 0;
}