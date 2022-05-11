//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include "maintenance_manager.h"

int MaintenanceManager()
{

    #ifdef DEBUG
    //printf("Maintenance Manager!!!\n");
    #endif
    signal(SIGINT,sigint_maintenance);


    list_pids = malloc(sizeof(pid_t)*SMV->EDGE_SERVER_NUMBER);
    int edgeserver_to_maintenance; 
    msg work_msg;
    char buf[512];

    //receber informaçao dos edge servers a dizer que estão a trablhar
    for(int i = 0;i<SMV->EDGE_SERVER_NUMBER;i++){
        //LER MENSAGEM DA MQ
        msgrcv(SMV->msqid,&work_msg, sizeof(work_msg) - sizeof(long),i+1,0);

        list_pids[i] = work_msg.msg_content;
    }

    write_screen_log("MAINTENANCE MANAGER: ALL EDGE SERVERS READY TO WORK");

    //work
    while(1){

        //Sleep
        sleep( 1 + rand()%5);


        edgeserver_to_maintenance = rand()% SMV->EDGE_SERVER_NUMBER;

        //prepare message
        work_msg.msgtype = list_pids[edgeserver_to_maintenance];
        work_msg.msg_content = 0;


        //write na MQ
        snprintf(buf,sizeof(buf),"SENDING EDGE SERVER %d TO MAINTENANCE",edgeserver_to_maintenance);
        write_screen_log(buf);
        msgsnd(SMV->msqid,&work_msg,sizeof(work_msg) - sizeof(long),0);


        //warn edge server 
        pthread_cond_broadcast(&SMV->edge_server_move);

        //wait for edge server saying that is ready for maintenance
        msgrcv(SMV->msqid,&work_msg, sizeof(work_msg) - sizeof(long),list_pids[edgeserver_to_maintenance] + 50,0);

        //Do maintenance
        sleep( 1 + rand()%5);

        //Return to edge server saying that it can continue working normally
        work_msg.msg_content = 0;
        work_msg.msgtype = list_pids[edgeserver_to_maintenance];
        msgsnd(SMV->msqid,&work_msg,sizeof(work_msg) - sizeof(long),0);


    }
    
    free(list_pids);
    
    return 0;
}

void sigint_maintenance(){
    free(list_pids);
    exit(0);
}