//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "maintenance_manager.h"

int MaintenanceManager()
{
    signal(SIGINT,SIG_DFL);

    #ifdef DEBUG
    printf("Maintenance Manager!!!\n");
    pause();
    #endif
    
    return 0;
}