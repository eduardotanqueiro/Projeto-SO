#include "mainHeader.h"

int main(int argc, char** argv){

    //Redirect CTRL-C
    signal(SIGINT,sigint);

    if(argc == 2)
        init(argv[1]); 
               
    else
    {
        fprintf(stderr,"Wrong command format\n");
        return -1;
    }


    return 0;
}

void cleanup(){




}

void write_screen_log(char* str){

    FILE* flog = fopen("log.txt","a");
    time_t now;
    struct tm timenow;

    //pthread_mutex_lock(&SMV->log_write_mutex);
    sem_wait(SMV->log_write_mutex);


    time(&now);
    localtime_r(&now,&timenow);
    fprintf(flog,"%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);
    printf("%02d:%02d:%02d %s\n",timenow.tm_hour,timenow.tm_min,timenow.tm_sec,str);

    sem_post(SMV->log_write_mutex);
    //pthread_mutex_unlock(&SMV->log_write_mutex);

    fclose(flog);

}

void sigint(){

    write_screen_log("Cleaning up resources");
    cleanup();
    write_screen_log("Cleanup complete! Closing system");


}