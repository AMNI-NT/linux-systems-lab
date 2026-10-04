#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <signal.h> 


int standard_command(char** arglist, int count){
	pid_t pid = fork();
	if(pid < 0){
		perror("fork failed");
		exit(1);
	}
	else if(pid == 0){
		// child process
		signal(SIGINT, SIG_DFL);
		execvp(arglist[0], arglist);
		perror("execvp failed");
		exit(1);
	}
	else{
		// parent process
		waitpid(pid,NULL, 0);
	}
	return 0;
}

// NEED TO UPDATE TO NOT ASSUME LOCATION OF > AND < IN ARGUMENTS
int output_redirecting(char** arglist, int count){

	int filename_index = 2; // default index of filename if > is the second argument
	for(int i = 0; i < count; i++){
		if(strcmp(arglist[i], ">") == 0){
			arglist[i] = NULL; // remove the > from the argument list
			filename_index = i + 1;
			break;
		}
	}

	char* filename = arglist[filename_index];
	pid_t pid = fork();
	
	if(pid < 0){
		perror("fork failed");
		return 1;
	}
	else if(pid == 0){
		// child process
		signal(SIGINT, SIG_DFL);
		int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);

		if(fp == -1){
			perror("open failed");
			exit(1);
		}
		//fsync(fp);
		dup2(fp, STDOUT_FILENO);
		
		close(fp);
		execvp(arglist[0], arglist);
		perror("execvp failed");
		exit(1);
	}
	else{
		// parent process
		waitpid(pid,NULL, 0);
	}

	return 0;
}

int input_redirecting(char** arglist, int count){
	// extract filename 
	int filename_index = 2; // default index of filename if < is the second argument
	for(int i = 0; i < count; i++){
		if(strcmp(arglist[i], "<") == 0){
			arglist[i] = NULL; // remove the < from the argument list
			filename_index = i + 1;
			break;
		}
	}

	char* filename = arglist[filename_index];
	pid_t pid = fork();
	
	if(pid < 0){
		perror("fork failed");
		return 1;
	}
	else if(pid == 0){
		// child process
		
		int fp = open(filename, O_RDONLY, 0600);
		if(fp == -1){
			perror("open failed");
			exit(1);
		}
		dup2(fp, STDIN_FILENO);

		close(fp);

		signal(SIGINT, SIG_DFL);
		execvp(arglist[0], arglist);
		perror("execvp failed");
		exit(1);
	}
	else{
		// parent process
		waitpid(pid,NULL, 0);
	}

	return 0;	
}

int ex_in_background(char** arglist, int count){
	pid_t pid = fork();
	if(pid < 0){
		perror("fork failed");
		exit(1);
	}
	else if(pid == 0){
		// child process
		arglist[count-1] = NULL; // remove the & from the argument list
		execvp(arglist[0], arglist);
		perror("execvp failed");
		exit(1);
	}
	// parent process does not wait for child to finish
	return 0;
}

int multiple_piping(char** arglist, int count){

	int command_start_index[count];

	// calculate number of commands and where in arglist they are
	int num_commands = 0; 
	command_start_index[num_commands++] = 0;

	for(int i = 0; i < count; i++){
		if(strcmp(arglist[i], "|") == 0 ){
			arglist[i] = NULL;
			command_start_index[num_commands++] = i + 1;
		}	
	}

	if(num_commands > 10){
		fprintf(stderr, "too many number commands for piping");
		return 1;
	}

	// create pipes
	int pipes[num_commands-1][2]; 
	for(int i = 0; i < num_commands-1; i++){
		if(pipe(pipes[i]) == -1){
			perror("pipe failed");
			exit(1);
		}
	}

	pid_t pids[num_commands];

	// fork children
	for(int i = 0; i < num_commands; i++){
		pids[i] = fork();
		if(pids[i] < 0){
			perror("fork failed");
			for(int j = 0; j < num_commands-1; j++){
				close(pipes[j][0]);
				close(pipes[j][1]);
			}
			exit(1);
		}
		else if(pids[i] == 0){
			
			if(i > 0)
				dup2(pipes[i-1][0], STDIN_FILENO); // redirect stdin to read end of previous pipe
			if(i < num_commands-1)
				dup2(pipes[i][1], STDOUT_FILENO); // redirect stdout to write end of current pipe

			// close all pipes in child
			for(int j = 0; j < num_commands-1; j++){
				close(pipes[j][0]);
				close(pipes[j][1]);
			}

			signal(SIGINT, SIG_DFL); 
			execvp(arglist[command_start_index[i]], &arglist[command_start_index[i]]);
			perror("execvp failed");
			exit(1);
		}
	}

	// parent proccess closes all pipes
	for(int j = 0; j < num_commands-1; j++){
		close(pipes[j][0]);
		close(pipes[j][1]);
	}

	// wait for children to finish
	for(int i = 0; i < num_commands; i++){
		waitpid(pids[i], NULL, 0);
	}
	return 0;
}


// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
	
	char* types[] = {"<", ">", "|", "&"};
	int type_idx = -1;

	// check for type of command and call appropriate function to handle it
	if( strcmp(arglist[count-1], "&") == 0){
		type_idx = 3;
	}
	else{
		for(int i = 0; i < count; i++){
			for(int j = 0; j < 4; j++){
				if(strcmp(arglist[i], types[j]) == 0){
					type_idx = j;
					goto found_type;
				}
			}
		}
	}

	found_type:
	switch(type_idx){
		case 0: // input redirecting
			if(input_redirecting(arglist, count) != 0)
				return 1;	
			break;
		case 1: // output redirecting
			if(output_redirecting(arglist, count) != 0)
				return 1;
			break;
		case 2: // multiple piping
			if(multiple_piping(arglist, count) != 0)
				return 1;
			break;
		case 3: // background process
			if(ex_in_background(arglist, count) != 0)
				return 1;
			break;
		default: // standard command
			if(standard_command(arglist, count) != 0)
				return 1;
			break;
	}
	return 1;
}

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
	signal(SIGCHLD, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	return 0;
}
int finalize(void){
	return 0;
}