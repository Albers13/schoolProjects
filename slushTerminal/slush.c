//Logan Albers Alejandro Casillo Adam Ezzelgot
//2/11/2020
//creates a terminal called slush

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#define BUFFERSIZE 4096 

//gets cwd
void getPath() {
        char cwd[PATH_MAX];
        char* curDir = getcwd(cwd, sizeof(cwd));
        char* homeDir = getenv("HOME");
        curDir += strlen(homeDir);
        printf("slush|%s> ", curDir);
}

//changes directory
void changeDirectory(char* args[]) {
    if(args[0] == NULL){
        printf("Not a valid input \n");
    }
    else {
        int valDir = chdir(args[0]);
        if (valDir !=0){
            perror("slush");
        }
    }
}

void catch(int signalNum) {
    printf("\n");
}

int main(int argc, char* argv[]) { 
    int max_args = 15;
    int max_slush_size = max_args + 2;
    char* cmd;
    char* slush_args[max_slush_size];
    char inputString[BUFFERSIZE];
    int boolean = 1;
    int nFD[2];
    int oFD[2];
    while(boolean == 1){
	//prints the handle for slush (the current directory)
	getPath();
        signal(2, catch);

        //gets the user input arguments
        char* isDone = fgets(inputString, BUFFERSIZE, stdin);

        //checks for the ^d to terminate slush
        if(isDone == NULL) {
            printf("\n");
            boolean = 0;
            exit(-1);
        }

        char* holder = strtok(inputString, "\n");
	char* slush_cmds[128]; //allows for 256 characters
        char* cmdTemp = strtok(holder, "(");
        slush_cmds[0] = cmdTemp;
	int len = 1;	
        //parses the user input
        while (cmdTemp != NULL) {
            cmdTemp = strtok(NULL, "(");
            slush_cmds[len] = cmdTemp;
            if(cmdTemp != NULL) {
                len++;
            }
        }
        
	int i;
	 //for parsing and executing (piping)
        for (i = len - 1; i >= 0; i--) {
            cmd = strtok(slush_cmds[i], " ");
            if (cmd == NULL) {
                printf("Not a valid input! \n"); 
                break;
            }

            slush_args[0] = cmd;
            char* slushParser = slush_args[0];
	    int counter = 1;

            while (slushParser != NULL) {
                slushParser = strtok(NULL, " ");
                slush_args[counter] = slushParser;
                if (slushParser != NULL){
                    counter++;
                }
                slush_args[counter + 1] = NULL;
            }

            //checks for cd
            if (len == 1) {
                if (!strcmp(cmd, "cd")) {
                        changeDirectory(&slush_args[1]);
                }
                else {
                    int child1 = fork();
                    if (child1 == -1) {
                        perror("Error trying to fork child1\n");
                    }
                    //child1
                    if (child1 == 0) {  
                            int retVal1 = execvp(cmd, slush_args);
                            if (retVal1 == -1) {
                              perror("Error trying to execute retVal1 \n");
			    }
                    }
                    //parent of child1
                    else {
                        waitpid(child1, NULL, 0);
                    }
                }
            }
            else {
                //first command of the arguments
                if (i == (len - 1)) {
                    pipe(nFD);
                    int child2 = fork();
                    if (child2 == -1) {
                        perror("Error trying to fork child2 \n");
                    }
                    //child2
                    if (child2 == 0) {
                        dup2(nFD[1], STDOUT_FILENO);
                        close(nFD[0]);
                        int retVal2 = execvp(cmd, slush_args);
                        if (retVal2 == -1) {
                          perror("Error trying to execute retVal2\n");
                        }
                    }
                    //parent of child2
                    else {
                        waitpid(child2, NULL, 0);
                    }
                    oFD[0] = nFD[0];
                    close(nFD[1]);
                }
                //last command of the arguments
                else if (i == 0) {
                    int child3 = fork();
                    if (child3 == -1) {
                        perror("Error trying to fork child3");
                    }
                    //child3
                    if (child3 == 0) {
                        dup2(oFD[0], STDIN_FILENO);
                        close(oFD[1]);
                        int retVal3 = execvp(cmd, slush_args);
                        if (retVal3 == -1) {
                          perror("Error trying to execute retval3\n");
                        }
                    }
                    //parent of child3
                    else {
                        close(nFD[0]);
                        close(oFD[0]);
                        close(nFD[1]);
                        close(oFD[1]);
                        waitpid(child3, NULL, 0);
                    }
                }
                //for middle commands of the arguments
                else {
                    pipe(nFD);
                    int child4 = fork();
                    if (child4 == -1){
                        perror("Error trying to fork child4 \n");
                    }
                    //child 4
                    if (child4 == 0) {
                        dup2(oFD[0], STDIN_FILENO);
                        close(oFD[1]);
                        dup2(nFD[1], STDOUT_FILENO);
                        close(nFD[0]);
                        int retVal4 = execvp(cmd, slush_args);
                        if (retVal4 == -1) {
                          perror("Error trying to execute retVal4 \n");
                        }
                    }
                    //parent of child4
                    else {
                        close(oFD[0]);
                        oFD[0] = nFD[0];
                        close(nFD[1]);
                        waitpid(child4, NULL, 0);
                    }
                }
            }
        }
    }
    return 0;
}

