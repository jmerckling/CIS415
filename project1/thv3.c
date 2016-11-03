//Jordan Merckling, jmerckli
//CIS 415 Project 1
//This is my own work

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "p1fxns.h"

#define BUF_SIZE 1024
#define MAX_PIDS 1024
#define MAX_ARGS 512
#define INTERVAL 250
#define UNUSED __attribute__((unused))


pid_t deadch[BUF_SIZE];
int killcount;
int nprocessors;
int nprocesses;
pid_t pid[MAX_ARGS];



int USR1_received = 0;

void USR1_handler(UNUSED int signo)
{
    USR1_received++;
}

typedef struct processor
{
    pid_t pid;
    struct processor *next;
    int status;
} pr;

typedef struct queue
{
    pr *head;
    pr *tail;
    int size;
} Queue;

pr* deq(Queue*);
void enqueue(Queue*, pr*);

Queue rQ;
Queue wQ;

void ALRM_handler(UNUSED int signo)
{
    int i;
    for(i = 0; i < nprocesses; i++)
    {
        pr* ptemp = malloc(sizeof(pr));
        ptemp->pid = pid[i];
        ptemp->status = 0;
        enqueue(&wQ, ptemp);
        free(ptemp);
    }

    for(i = 0; i < nprocesses; i++)
    {
        if(wQ.size == 0)
            p1perror(2, "wait queue empty");
        pr* t = deq(&wQ);
        kill(pid[i], SIGUSR1);
        if(t->status == 0)
        {
            t->status = 1;
            enqueue(&rQ, t);
        }
        else
        {
            kill(pid[i], SIGCONT);
        }
    }

    int f = 0;
    for(i = 0; i < rQ.size; i++)
    {
        pr *node = deq(&rQ);
        int k;
        for(k = 0; k < killcount; k++)
        {
            if(deadch[k] == node->pid)
            {
                f = 1;
            }
        }
        if(f == 0)
        {
            enqueue(&wQ, node);
        }
        else
        {
            free(node);
        }
    }
}

static void CHLD_handler(UNUSED int signo)
{
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            killcount++;
        }
    }
}

void enqueue(Queue* queue, pr *process)
{
    pr *cur = process;
    if(queue == NULL)
    {
        queue->head = cur;
        queue->tail = cur;
        queue->size = queue->size+1;
        return;
    }
    else
        queue->tail->next = cur;
    queue->tail = cur;
    queue->size = queue->size+1;
}

pr* deq(Queue *queue)
{
    if(queue->head == NULL)
    {
        return NULL;
    }
    else if (queue->head == queue->tail)
    {
        pr *cur = queue->head;
        queue->size = queue->size-1;
        queue->head = NULL;
        queue->tail = NULL;
        return cur;
    }
    else
    {
        pr *cur = queue->head;
        queue->head = cur->next;
        queue->size = queue->size-1;
        return cur;
    }
}

int main(int argc,char *argv[])
{
    int i;
    struct timeval start, stop;
    struct itimerval it_val;
    char* command;
    int proc_nums = 0;
    int proc_pro = 0;

    rQ.head = NULL;
    rQ.tail = NULL;
    rQ.size = 0;

    wQ.head = NULL;
    wQ.tail = NULL;
    wQ.size = 0;

    it_val.it_value.tv_sec = INTERVAL/1000;
    it_val.it_value.tv_usec = (INTERVAL*1000) % 1000000;
    it_val.it_interval = it_val.it_value;

    if(signal(SIGUSR1, USR1_handler) == SIG_ERR)
    {
        return -1;
    }

    for(i = 1; i < argc; i++)
    {
        if(p1strneq(argv[i], "--command=",p1strlen("--command=")))
        {
            // need to set allocation
            command = (char *) malloc((p1strlen(argv[i] + 1))*sizeof(char));
            p1strcpy(command, argv[i] + p1strlen("--command="));
        }
        else if(p1strneq(argv[i], "--number=",  p1strlen("--number=")))
        {
            nprocesses = p1atoi(argv[i] + p1strlen("--number="));
            proc_nums = 1;
        }
        else if(p1strneq(argv[i], "--processors=", p1strlen("--processors=")))
        {
            nprocessors = p1atoi(argv[i] + p1strlen("--processors="));
            proc_pro = 1;
        }
    }
    if(command == NULL)
    {
        p1perror(1, "No command given");
        _exit(2);
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
        setitimer(ITIMER_REAL, &it_val,NULL);
        if(setitimer(ITIMER_REAL, &it_val, NULL) == -1)
        {
            return 1;
        }
        pid[i] = fork();
        if(pid[i] == 0)
        {

            execvp(arg[0], arg);
        }
    }

    for(i = 0; i < nprocesses; i++)
    {
        waitpid(pid[i], NULL, 0);
    }

    char ms[10];
    ms[0] = 0;
    gettimeofday(&stop, NULL);
    float t = (((stop.tv_sec - start.tv_sec) * 1000000L + stop.tv_usec) - start.tv_usec)/1000000.0;
    int perLeft = (((stop.tv_sec - start.tv_sec)*1000000L + stop.tv_usec) - start.tv_usec)/1000000;
    int perRight = (((stop.tv_sec - start.tv_sec)*1000000L + stop.tv_usec) - start.tv_usec)/1000 - perLeft * 1000;
    
    p1putstr(1, "The elapsed time to execute ");
    p1putint(1, nprocesses);
    p1putstr(1, " copies of \"");
    p1putstr(1, command);
    p1putstr(1, "\" on ");
    p1putint(1, nprocessors);
    p1putstr(1, " processors is: ");
    p1putint(1, perLeft);
    p1putstr(1, ".");
    if(perRight < 100)
    {
    p1putstr(1,(char*)"0");
	if(perRight < 10)
	{
	    p1putstr(1, (char*)"0");
	}
    }
    p1itoa(perRight, ms);
    p1putstr(1,ms);
    p1putstr(1, "sec\n");


    free(command);
    return 1;
}//end main




