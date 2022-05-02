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


    int edgeserver_to_maintenance; 
    msg work_msg;

    //work
    while(1){

        edgeserver_to_maintenance = rand()% SMV->EDGE_SERVER_NUMBER;

        //prepare message
        work_msg.msgtype = edgeserver_to_maintenance;
        work_msg.msg_flag = 0;

        //write na MQ
        msgsnd(SMV->msqid,&work_msg,sizeof(work_msg) - sizeof(long),0);

        //wait for edge server saying that is ready for maintenance
        msgrcv(SMV->msqid,&work_msg, sizeof(work_msg) - sizeof(long),edgeserver_to_maintenance,0);

        //Do maintenance
        sleep( 1 + rand()%5);

        //Return to edge server saying that it can continue working normally
        work_msg.msg_flag = 2;
        msgsnd(SMV->msqid,&work_msg,sizeof(work_msg) - sizeof(long),0);

        //Sleep
        sleep( 3 + rand()%5);
    }
    
    
    return 0;
}