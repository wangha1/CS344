#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define MAX_ARGUMENTS 512
#define BG_MAX 100

void runloop();
int parse(char *line, char **arguments);
int  execute(char **arguments, pid_t *child_list, int *lastStatus, int marker);
int changeDirectory(char **arguments);
int exitSh(int *child_list);
int checkSignal(int *lastStatus);
int  execArguments(char **arguments, pid_t *child_list, int *lastStatus, int marker);
void push_child_list(pid_t *child_list, pid_t pid);
void check_background(pid_t *child_list);
int redirect(char **arguments, int putType, int background);

int main(){
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = SIG_IGN;
	sigaction(SIGINT, &act, NULL);


	runloop();

	return 0;
}

void runloop(){  //here's main loop functio
	char *line;
	char **args;
	int i, status, marker;
	pid_t *child_list = malloc(sizeof(pid_t) * BG_MAX);//here is children list
	int *lastStatus = malloc(sizeof(int));

	for(i = 0; i < BG_MAX; i++)
		child_list[i] = 0;

	do{
	
		check_background(child_list);//check state everytime jump into loop
		printf(": ");
		
		args = malloc(sizeof(char*) * MAX_ARGUMENTS);
		line = malloc(BUFFER_SIZE);
		
		fflush(stdout);		

		fgets(line, BUFFER_SIZE, stdin);

		marker = parse(line, args);
		
		status = execute(args, child_list, lastStatus, marker);// execute after parse

		free(line);
		free(args);

	}while(status); //run till exit
}

int parse(char *buffer, char **arguments){ //parse here, will return lenth of arguments

	int counter = 0;

	arguments[counter] = strtok(buffer, " \n");

	while(arguments[counter] != NULL){
		counter++;
		arguments[counter] = strtok(NULL, " \n"); //parse and save arguments
	}
	
	counter--;
	return counter;
}

int  execute(char **arguments, pid_t *child_list, int *lastStatus, int marker){
	if(arguments[0] == NULL || strcmp(arguments[0], "#") == 0)
		return 1;// comment, do nothing

	else if(strcmp(arguments[0], "cd") == 0)
		return changeDirectory(arguments);
	
	else if(strcmp(arguments[0], "status") == 0)
		return checkStatus(lastStatus);

	else if(strcmp(arguments[0], "exit") == 0)
		return exitSh(child_list);

	else
		return execArguments(arguments, child_list, lastStatus, marker);
}
//cd command
int changeDirectory(char **arguments){

	if(arguments[1] == NULL)
		chdir(getenv("HOME"));

	else
		chdir(arguments[1]);

	return 1;
}
//exit command
int exitSh(pid_t *child_list){
	int i;
	for(i = 0; i < BG_MAX; i++){
		if(child_list[i] > 0)
			kill(child_list[i], SIGKILL);
	}
	free(child_list);

	return 0;
}
//status command
int checkStatus(int *lastStatus){
	int status;
	
	status = *lastStatus;

	if(WIFEXITED(status))
		printf("Exited value: %d.\n", WEXITSTATUS(status));

	else if(WIFSIGNALED(status))	
		printf("terminated by signal %d.\n", WTERMSIG(status));
	return 1;
}
//none build-in command will deal in here
int execArguments(char **arguments, pid_t *child_list, int *lastStatus, int marker){
	pid_t pid, wpid;
	int status;

	pid = fork();											
	int fd, putType = -1, background = 0;

	if(strcmp(arguments[marker], "&") == 0){
		background = 1;//is a background
		arguments[marker] = NULL;
		marker--;
		push_child_list(child_list, pid);//push into list
	}
	
	if(arguments[1] != NULL || background == 1){
		//output if putType == 1
		//input if putType == 0
		//no such file if putType == -1
		//just a flag

		if(strcmp(arguments[1], ">") == 0)
			putType = 1;
 	
		if(strcmp(arguments[1], "<") == 0)
			putType = 0;
					
		fd = redirect(arguments, putType, background);	
	}
	
	//children process
	if(pid == 0){
		if(background == 1)
			signal(SIGINT, SIG_IGN);


		else
			signal(SIGINT, SIG_DFL);
		//just open the file below

		if(putType == 1)
			dup2(fd, 1);

		else if(putType == 0)
			dup2(fd, 0);

		if(execvp(arguments[0], arguments) == -1){
			perror(arguments[0]);
			exit(1);
		}
	}

	else if(pid < 0)
		printf("Error Forking\n");
	//parent!!
	else{
		if(background == 0){
			// if it is background, wait till it finished
			do{
				wpid = waitpid(pid, &status, WUNTRACED);
			}while(!WIFEXITED(status) && !WIFSIGNALED(status));

			*lastStatus = status;		

			if(WIFSIGNALED(status)){
				printf("Terminated by Signal %d\n", WTERMSIG(status));
				fflush(stdout);	
			}
		}
		
		else{
			printf("Background Pid is: %d\n", pid);
			fflush(stdout);
		}

	}

	return 1;
}	


//this function will check if background process have finished
void check_background(pid_t *child_list){
	int i, status;
	pid_t pid = waitpid(-1, &status, WNOHANG);

	if(pid > 0){
		for(i = 0; i < BG_MAX; i++){
			if(child_list[i] == pid){
				printf("Background pid #%d is killed: ", pid);
	
				if(WIFEXITED(status))
					printf("Exited value: %d.\n", WEXITSTATUS(status));
		
				else if(WIFSIGNALED(status))	
					printf("terminated by signal %d.\n", WTERMSIG(status));
		
				child_list[i] = 0;

				return;
			}
		}
	}
}

void push_child_list(pid_t *child_list, pid_t pid){
	int i;
	
	for(i = 0; i < BG_MAX; i++){
		if(child_list[i] == 0){
			child_list[i] = pid;
			return;
		}
	}
}		

int redirect(char **arguments, int putType, int background){
	int fd;
	
	if(putType == 1){
		fd = open(arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0664);
	
		if(fd == -1){
			perror(arguments[2]);
			exit(1);
		}

		arguments[1] = NULL;
		arguments[2] = NULL;
	}

	else if(putType = 0){
		fd = open(arguments[2], O_RDONLY);

		if(fd == -1){
			perror(arguments[2]);
			exit(1);
		}

		arguments[1] = NULL;
		arguments[2] = NULL;
	}
	
	if(background == 1) {
		if(putType != 1)
			fd = open("/dev/null", O_RDONLY);
	
		if(putType != 0)
			fd = open("/dev/null", O_WRONLY);
	}

	return fd;			
}
		
	
