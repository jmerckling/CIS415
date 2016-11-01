#include <sys/types.h>
#include <unistd.h>
#include "p1fxns.h"
#include <signal.h>

#define UNUSED __attribute__ ((unused))

void sig_handler(UNUSED int signo) {
}

int main(UNUSED int argc, UNUSED char *argv[]) {
    sigset_t set;
    siginfo_t info;
    pid_t pid;

    pid = getpid();
    p1putint(1, pid);
    p1putstr(1, ": my pid\n");
    if (signal(SIGUSR1, sig_handler)== SIG_ERR)
        p1putstr(1, "cant access SIGUSR1\n");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    p1putstr(1, "Waiting for SIGUSR1\n");
    sigwaitinfo(&set, &info);
    p1putstr(1, "SIGUSR1 Received\n");
    return 0;
}

