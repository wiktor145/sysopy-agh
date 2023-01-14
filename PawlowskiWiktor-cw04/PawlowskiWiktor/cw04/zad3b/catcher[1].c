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

int kill_counter = 0;
int kill_while = 1;
int kill_pid;

void kill_USR1(int n, siginfo_t *siginfo,void *ucontext)
{
    kill_counter++;
    kill_pid=siginfo->si_pid;
    kill(kill_pid,SIGUSR1);
}

void kill_USR2(int n, siginfo_t *siginfo,void *ucontext)
{
    kill_while=0;

}


void kill_catch(void)
{
    struct sigaction act;
    act.sa_sigaction = kill_USR1;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigdelset(&act.sa_mask,SIGUSR1);
    sigdelset(&act.sa_mask,SIGUSR2);

    struct sigaction act1;
    act1.sa_sigaction = kill_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGUSR1);
    sigdelset(&act1.sa_mask,SIGUSR2);
    act1.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act1, NULL);

    while (kill_while)
    {
        pause();
    }


    kill(kill_pid,SIGUSR2);
    printf("Got: %d\n",kill_counter);
    exit(0);

}


int queue_counter = 0;
int queue_while = 1;
int queue_pid;
union sigval sig;

void queue_USR1(int n, siginfo_t *siginfo,void *ucontext)
{
    queue_counter++;
    queue_pid=siginfo->si_pid;
    sig.sival_int=queue_counter;
    sigqueue(queue_pid,SIGUSR1,sig);

}

void queue_USR2(int n, siginfo_t *siginfo,void *ucontext)
{
    queue_while=0;

}


void queue_catch(void)
{
    struct sigaction act;
    act.sa_sigaction = queue_USR1;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigdelset(&act.sa_mask,SIGUSR1);
    sigdelset(&act.sa_mask,SIGUSR2);

    struct sigaction act1;
    act1.sa_sigaction = queue_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGUSR1);
    sigdelset(&act1.sa_mask,SIGUSR2);
    act1.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act1, NULL);

    while (queue_while)
    {
        pause();
    }


    sig.sival_int=queue_counter;
    sigqueue(queue_pid,SIGUSR2,sig);
    printf("Got: %d\n",queue_counter);
    exit(0);

}


void real_catch(void)
{
    struct sigaction act;
    act.sa_sigaction = kill_USR1;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigdelset(&act.sa_mask,SIGRTMIN);
    sigdelset(&act.sa_mask,SIGRTMIN+1);

    struct sigaction act1;
    act1.sa_sigaction = kill_USR2;
    sigfillset(&act1.sa_mask);
    sigdelset(&act1.sa_mask,SIGRTMIN);
    sigdelset(&act1.sa_mask,SIGRTMIN+1);
    act1.sa_flags = SA_SIGINFO;

    sigaction(SIGRTMIN, &act, NULL);
    sigaction(SIGRTMIN+1, &act1, NULL);

    while (kill_while)
    {
        pause();
    }

    for (int i=0; i<kill_counter; i++)
    {
        //printf("Send\n");
        kill(kill_pid,SIGRTMIN);
    }


    kill(kill_pid,SIGRTMIN+1);
    printf("Got: %d\n",kill_counter);
    exit(0);

}





int main(int argc, char**argv)
{
    if (argc!=2)
    {
        printf("Wrong nr of arguments\n");
        printf("You should give mode: 1 - kill\n ");
        printf("2 - SIGQUEUE     3 - SIGRT\n");
        exit(1);

    }

    int mode = strtol(argv[1],NULL,10);
    if (mode != 1 && mode != 2 && mode != 3)
    {
        printf("Wrong mode, should be 1 2 or 3\n");
        exit(1);
    }

    printf("My PID: %d\n",getpid());


    if (mode == 1)
    {
        kill_catch();

    }
    else if (mode == 2)
    {
        queue_catch();
    }
    else
    {
        real_catch();
    }

    return 0;

}