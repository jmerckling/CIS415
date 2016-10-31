//Jordan Merckling, jmerckli
//CIS 415 Project 1
//This is my own work

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
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

    for(i = 1; i < argc; i++)
        {
           	if(p1strneq(argv[i], "--command=",p1strlen("--command=")))
            {
              	// need to set allocation
		    	command = (char *) malloc((p1strlen(argv[i] + 1))*sizeof(char));
                p1strcpy(command, argv[i] + p1strlen("--command="));
		   		//printf("Command: %s\n", command);//delete
            }
			else if(p1strneq(argv[i], "--number=",  p1strlen("--number=")))
			{
		    	nprocesses = p1atoi(argv[i] + p1strlen("--number="));
				//printf("Processes: %d\n", nprocesses);//delete
			}
			else if(p1strneq(argv[i], "--processors=", p1strlen("--processors=")))
			{
		   	 	nprocessors = p1atoi(argv[i] + p1strlen("--processors="));
				//printf("Processors: %d\n", nprocessors);//delete			    
			}    
        }
		
		//if no argument is given for processes set it to TH_NPROCESSES
		if(nprocessors < 1)
		{
			char *x;		
            if ((x = getenv("TH_NPROCESSORS")) != NULL)
               { 
				   printf("%s\n", x);
				   nprocessors = p1atoi(x);
			   }
			else
				{
					p1perror(1, "No value found for nprocessors");
					_exit(1);
				}
		}
		//if no argument is given for processes set it to TH_NPROCESSES
		if(nprocesses < 1)
		{
			char *x;		
            if ((x = getenv("TH_NPROCESSES")) != NULL)
               { 
				   printf("%s\n", x);
				   nprocesses = p1atoi(x);
			   }
			else
				{
					p1perror(1, "No value found for nprocesses");
					_exit(1);
				}
		}
		gettimeofday(&start, NULL);
		
		for(i = 0; i < nprocesses; i++)
		{
			pid[i] = fork();
			if(pid[i] == 0)
			{
				execvp(arg[0], arg);
			}
		}
		
		for(i = 0; i < nprocesses; i++)
		{
			system(command);
			int status;
			waitpid(pid[i], &status, 0);
		}
		
		//printf("Command: %s\n", command);//delete
		//printf("Processes: %d\n", nprocesses);//delete
		//printf("Processors: %d\n", nprocessors);//delete
		gettimeofday(&stop, NULL);
		double t = stop.tv_usec - start.tv_usec;
		printf("The elapsed time to execute %d copies of \\”%s\\” on %d processors is: %7.3fsec\n", nprocesses, command, nprocessors, t);
		//write()
		
		
    free(command);
}//end main
 	
