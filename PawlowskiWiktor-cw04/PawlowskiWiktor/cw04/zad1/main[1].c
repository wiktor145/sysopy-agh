//
// Created by wicia on 27.03.19.
//
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int prev_tstp = 0; //for distinguishing between first and second
    //ctrl-z

void handle_int(int signum)
{
    printf("\nOdebrano sygnal SIGINT\n");
    exit(0);
}

void handle_tstp(int sig_no)
{
    if (prev_tstp == 0)
    {
        printf("\nOczekuje na: \nCTRL+Z - kontynuacja, albo\n");
        printf("CTRL+C - zakonczenie programu\n");
        prev_tstp=1;
        pause();
    }
    else
    {
        printf("\nPowrot do dzialania\n");
        prev_tstp=0;
    }

}

int main (void)
{
    time_t czas;
    char timebuff[20];
    struct sigaction act;
    act.sa_handler = handle_tstp;
    sigfillset(&act.sa_mask);  // block all signals
    sigdelset(&act.sa_mask,SIGTSTP); //apart from this one
    sigdelset(&act.sa_mask,SIGINT); //and this one

    act.sa_flags = SA_NODEFER;  //do not block sigtstp during its handling
    sigaction(SIGTSTP, &act, NULL);

    signal(SIGINT,handle_int);

    while (1)
    {
        time(&czas);
        strftime(timebuff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&czas));
        printf("%s\n",timebuff);
        sleep(1);
    }
    return 0;
}