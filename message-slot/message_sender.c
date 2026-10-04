#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "message_slot.h"  

int main(int argc, char *argv[]){

    if(argc < 5){
        perror("Too few arguments");
        exit(1);
    }
        

    char* path = argv[1];
    unsigned long channel_id = strtoul(argv[2], NULL, 10);
    int censorship_mode = atoi(argv[3]);
    char* message = argv[4];

    int fd = open(path, O_RDWR);
    if(fd < 0){
        perror("Openning file failed");
        exit(1);
    }
    
    if( ioctl(fd, MSG_SLOT_CHANNEL, channel_id)< 0){
        
        exit(1);
    }

    if( ioctl(fd, MSG_SLOT_SET_CEN, censorship_mode) < 0){
        
        exit(1);
    }
    
    

    size_t len = strlen(message);

    
    if( write(fd, message, len) < 0 ){
        
        exit(1);
    }

    close(fd);
    return 0;
}