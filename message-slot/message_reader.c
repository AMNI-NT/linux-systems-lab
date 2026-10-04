#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char *argv[]){

    char* path;
    int fd;
    char buffer[BUF_LEN];
    ssize_t n;
    unsigned long channel_id;

    if(argc < 3){
        perror("Too few arguments");
        exit(1);
    }
        
    path = argv[1];
    channel_id = strtoul(argv[2], NULL, 10);

    fd = open(path, O_RDWR);
    if(fd < 0){
        perror("Openning file failed");
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("ioctl failed");
        exit(1);
    }

    n = read(fd, buffer, BUF_LEN);

    if (n < 0) {
        perror("read failed");
        exit(1);
    }

    if(write(STDOUT_FILENO, buffer, n)<0){
        perror("write failed");
        exit(1);
    }

    close(fd);

    return 0;
}