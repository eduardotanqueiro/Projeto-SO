//  Eduardo Carneiro - 2020240332
//  Lucas Anjo - 2020218028

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STD
#define STD
#include "std.h"
#endif

#define BUFPIP 512
int fd_named_pipe;



int generate_request(int num_instructions, int timeout);

int main(int argc, char** argv){

    
    if(argc == 5){
        
        //Open TASK PIPE
        if ((fd_named_pipe = open(PIPE_NAME, O_WRONLY)) < 0) {
            perror("Cannot open pipe for writting: ");
            exit(1);
	    }


        //Variables
        int time_space = atoi(argv[2]);
        int num_instructions = atoi(argv[3]);
        int timeout = atoi(argv[4]);

        if( (time_space == 0) || (num_instructions == 0)|| (timeout == 0)){
            exit(1);
        }

        for(int i = 0; i< atoi(argv[1]); i++){

            
            if( generate_request(num_instructions,timeout) != 0 )
                printf("Error generating request number %d\n",i);

            usleep(time_space * 1000);
        }

    } 
    else
    {
        fprintf(stderr,"Wrong command format\n");
        return -1;
    }

    close(fd_named_pipe);
    printf("All requests placed successfully on the pipe!\nClosing mobile node...\n");
    return 0;
}


int generate_request(int num_instructions, int timeout){
    char buffer[BUFPIP];
    srand(time(0));
    
    snprintf(buffer,BUFPIP,"%d;%d;%d\n", 1 + rand()%500000 , num_instructions , timeout);

    if( write(fd_named_pipe,buffer,BUFPIP) < 0 ){
        perror("Write: ");
        return 1;
    }
    return 0;
}