#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "p1fxns.h"

#define BUF_SIZE 1024
#define MAX_PIDS 1024
#define MAX_ARGS 512

int main(int argc,char *argv[])
{
    int i;
    int nprocessors;
    int nprocesses;
    char buf[BUF_SIZE];
    struct timeval start, stop;
    char *arg[MAX_ARGS];
    char* command;
    pid_t pid[MAX_PIDS];

    int x;
    for(i = 1; i <= argc; i ++)
        {
           	if(p1strneq(argv[i], "--command=",p1strlen("--command=")))
                {
                  // need to set allocation
		    command = (char *) malloc((p1strlen(argv[i] + 1))*sizeof(char));
                   p1strcpy(command, argv[i] + p1strlen("--command="));
		   printf("Command: %s", command);
                }
		else if(p1strneq(argv[i], "--number=",  p1strlen("--number=")))
		{
		    nprocesses = p1atoi(argv[i] + p1strlen("--number="));
		}
		else if(p1strneq(argv[i], "--processors=", p1strlen("--processors=")))
		{
		    nprocessors = p1atoi(argv[i] + p1strlen("--processors="));			    
		}    

        }
    free(command);
}//end main
 	
