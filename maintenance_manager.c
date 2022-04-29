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

    //TODO receber informaçao dos edge servers a dizer que estão a trablhar
    for(int i = 0;i<SMV->EDGE_SERVER_NUMBER;i++){
        //LER MENSAGEM DA MQ
    }



    //work

    while(1){

        //write na MQ
        


        sleep( 1 + rand()%5);
    }
    
    return 0;
}