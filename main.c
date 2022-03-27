#include "sys_manager.h"

int main(int argc, char** argv){

    if(argc == 2)
        init(argv[1]); 
               
    else
    {
        fprintf(stderr,"Wrong command format\n");
        return -1;
    }


    return 0;
}