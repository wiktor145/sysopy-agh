#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>
#include <signal.h>

int kill_while = 1;
int kill_counter = 0;

void kill_USR1(int n)
{
    kill_counter++;
}

void kill_USR2(int n)
{
    kill_while=0;
}

void by_kill(int catcher_pid, int signals_no)
{
    struct sigaction act;
    act.sa_handler = kill_USR1;
    sigfillset(&act.sa_mask);
    sigdelset(&act.sa_mask,SIGUSR1);
    sigdelset(&act.sa_mask,SIGUSR2);
    act.sa_flags = 0;

    struct sigaction act1;
    act1.sa_handler = kill_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGUSR1);
    sigdelset(&act1.sa_mask,SIGUSR2);
    act1.sa_flags = 0;


    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act1, NULL);


    for (int i=0; i<signals_no;i++)
    {
        //printf("send\n");
        kill(catcher_pid,SIGUSR1);
    }

    kill(catcher_pid,SIGUSR2);

    while (kill_while)
    {
        pause();
    }

    printf("Should get: %d\n",signals_no);
    printf("Got: %d\n",kill_counter);
    exit(0);

}


int queue_while = 1;
int queue_counter = 0;
int queue_catcher_count=-1;

void queue_USR1(int n)
{
    queue_counter++;
}

void queue_USR2(int n, siginfo_t *siginfo,void *ucontext)
{
    queue_while=0;
    queue_catcher_count = siginfo->si_int;
}


void by_queue(int catcher_pid, int signals_no)
{
    struct sigaction act;
    act.sa_handler = queue_USR1;
    sigfillset(&act.sa_mask);
    sigdelset(&act.sa_mask,SIGUSR1);
    sigdelset(&act.sa_mask,SIGUSR2);
    act.sa_flags = 0;

    struct sigaction act1;
    act1.sa_sigaction = queue_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGUSR1);
    sigdelset(&act1.sa_mask,SIGUSR2);
    act1.sa_flags = SA_SIGINFO;


    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act1, NULL);

    union sigval sig;

    for (int i=0; i<signals_no;i++)
    {
        //printf("send\n");
        sigqueue(catcher_pid,SIGUSR1,sig);
    }

    sigqueue(catcher_pid,SIGUSR2,sig);

    while (queue_while)
    {
        pause();
    }

    printf("Should get: %d\n",signals_no);
    printf("Got: %d\n",queue_counter);
    printf("Catcher got: %d\n",queue_catcher_count);
    exit(0);

}


int real_while = 1;
int real_counter = 0;

void real_USR1(int n)
{
    real_counter++;
}

void real_USR2(int n, siginfo_t *siginfo,void *ucontext)
{
    real_while=0;

}


void by_real(int catcher_pid, int signals_no)
{
    struct sigaction act;
    act.sa_handler = real_USR1;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    struct sigaction act1;
    act1.sa_sigaction = real_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGRTMIN);
    sigdelset(&act1.sa_mask,SIGRTMIN+1);
    act1.sa_flags = SA_SIGINFO;


    sigaction(SIGRTMIN, &act, NULL);
    sigaction(SIGRTMIN+1, &act1, NULL);


    for (int i=0; i<signals_no;i++)
    {

        kill(catcher_pid,SIGRTMIN);
    }

    kill(catcher_pid,SIGRTMIN+1);

    while (real_while)
    {
        pause();
    }

    printf("Should get: %d\n",signals_no);
    printf("Got: %d\n",real_counter);
    exit(0);

}








int main(int argc, char** argv)
{


    if (argc != 4)
    {
        printf("Wrong number of arguments\n");
        printf("First should be PID of catcher\n");
        printf("Second is number of signals to send\n");
        printf("Third should be method of sending signals:\n");
        printf("1 - by kill,   2 - by SIGQUEUE,  3 - by SIGRT\n");
        exit(1);
    }

    int catcher_pid = strtol(argv[1],NULL,10);
    if (catcher_pid <=0)
    {
        printf("Error in PID\n");
        exit(1);
    }

    if (kill(catcher_pid,0) == -1)
    {
        printf("Process with given PID does not exist\n");
        exit(1);
    }

    int signals_no = strtol(argv[2],NULL,10);
    if (signal <=0)
    {
        printf("Error in signal number\n");
        exit(1);
    }

    int mode = strtol(argv[3],NULL,10);

    if (mode == 1)
    {
        by_kill(catcher_pid,signals_no);
    }
    else if (mode == 2)
    {
        by_queue(catcher_pid,signals_no);
    }
    else if (mode == 3)
    {
        by_real(catcher_pid,signals_no);
    }
    else
    {
        printf("Error in mode, should be 1,2 or 3\n");
        exit(1);
    }

    return 0;
}