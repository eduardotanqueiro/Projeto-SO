#include "mainHeader.h"

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

void write_screen_log(char* str){

    FILE* flog = fopen("log.txt","a");

    fprintf(flog,"%s\n",str);
    printf("%s\n",str);

    fclose(flog);

}