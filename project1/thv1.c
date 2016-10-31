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
    char *buf[BUF_SIZE];
    char word[MAX_ARGS];
    struct timeval start, stop;
   // char **arg[MAX_ARGS];
    char* command;
    pid_t pid[MAX_PIDS];
    int proc_nums = 0;
    int proc_pro = 0;

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
			proc_nums = 1;
				//printf("Processes: %d\n", nprocesses);//delete
			}
			else if(p1strneq(argv[i], "--processors=", p1strlen("--processors=")))
			{
		   	 	nprocessors = p1atoi(argv[i] + p1strlen("--processors="));
				proc_pro = 1;
				//printf("Processors: %d\n", nprocessors);//delete			    
			}
	}

		int j = 0;

		char **arg = malloc(sizeof(char*) * (p1strlen(command)+1));
		for(i = 0; i < p1strlen(command) + 1; i++)
		{
		    arg[i] = malloc(sizeof(char) * 100);
		}
		int command_location = 0;

		command_location = p1getword(command, command_location, arg[j]);

		while(command_location != -1)
		    {
			j++;
			command_location = p1getword(command, command_location, arg[j]);
		    }
		free(arg[j]);
		arg[j] = NULL;
		
		//if no argument is given for processes set it to TH_NPROCESSES
		if(proc_pro == 0)
		{
			char *x;		
            if ((x = getenv("TH_NPROCESSORS")) != NULL)
               { 
				  // printf("%s\n", x);
				   nprocessors = p1atoi(x);
			   }
			else
				{
					p1perror(1, "No value found for nprocessors");
					_exit(1);
				}
		}
		//if no argument is given for processes set it to TH_NPROCESSES
		if(proc_nums == 0)
		{
			char *x;		
            if ((x = getenv("TH_NPROCESSES")) != NULL)
               { 
				   //printf("%s\n", x);
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
			int status;
			waitpid(pid[i], &status, 0);
		}
		
	    	//printf("Command: %s\n", arg[0]);//delete
		//printf("Processes: %d\n", nprocesses);//delete
		//printf("Processors: %d\n", nprocessors);//delete
		gettimeofday(&stop, NULL);
		float t = (((stop.tv_sec - start.tv_sec) * 1000000L + stop.tv_usec) - start.tv_usec)/1000000.0;
	//	printf("The elapsed time to execute %d copies of \"%s\" on %d processors is: %7.3fsec\n", nprocesses, command, nprocessors, t);
		//display without printf
		p1putstr(1, "The elapsed time to execute ");
		p1putint(1, nprocesses);
		p1putstr(1, " copies of \"");
		p1putstr(1, command);
		p1putstr(1, "\" on ");
		p1putint(1, nprocessors);
		p1putstr(1, " processors is: ");
		p1putint(1, t);
		p1putstr(1, "sec\n");
		
		
    free(command);
}//end main
 	
