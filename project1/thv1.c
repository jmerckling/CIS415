#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#inlude <unistd.h>
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
    char *arg[MAX_ARGS]
    char command[BUF_SIZE];
    pid_t pid[MAX_PIDS]

    int x;
    for(i = 1; i <= argc; i ++)
        {
            if(p1strneq(argv[i], "--command=", 10))
                {
                   buf = argv[i]+p1strlen("--command=");
                   p1strcpy(buf, argv[i]+plstrlen("--command="));
                }
        }

    /*if(argc >= 2)
	{
	    if(p1strneq(argv[argc-1], "--command=", 10) != 1)
	    {
	        			
	    }
    	}
}	*/	
