#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**
    We report an error if the first file failed to close, 
    but still try to close the second one as well, before exiting
*/
void close_files(int fd_file_1, int fd_file_2){
    if(close(fd_file_1) == -1){
        perror("Closing file failed");
    }

    if(close(fd_file_2) == -1){
        perror("closing file failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int fd_copy_from;
    int fd_copy_to;
    int n;
    int ln=1;
    char* buffer;

    if(argc != 4){
        perror("Please provide 3 arguments\n");
        exit(1);
    }

    fd_copy_from = open(argv[1], O_RDONLY);
    if(fd_copy_from == -1){
        perror("Opening input file failed");
        exit(1);
    }

    fd_copy_to = open(argv[2], O_WRONLY);
    if(fd_copy_to == -1){
        perror("Opening output file failed");
        exit(1);
    }

    sscanf(argv[3], "%d", &n);

    if(n <= 0){
        perror("invalid buffer size");
        exit(1);
    }

    buffer = (char*) malloc(n);
    while(ln != 0){
        ln = read(fd_copy_from, buffer, n);
        if(ln == -1){
            perror("Reading file failed");
            close_files(fd_copy_from, fd_copy_to);
            free(buffer);
            exit(1);
        }
        if(write(fd_copy_to, buffer, ln) == -1){
            perror("Writing to file failed");
            free(buffer);
            close_files(fd_copy_from, fd_copy_to);
            exit(1);
        }
    }

    free(buffer);
    
    close_files(fd_copy_from, fd_copy_to);
    
    return 0;
}
