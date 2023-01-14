#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int child_is_alive = 0; //pretty obvious...
pid_t child;

void make_child(void)
{
    printf("\nTworzenie procesu potomnego\n");
    child = fork();
    if (child == -1)
    {
        printf("Error in creating child\n");
        exit(1);
    }

    if (child == 0)
    {
        execl("./skrypt.sh","skrypt.sh",NULL);
    }
    child_is_alive=1;

}

void handle_int(int signum)
{
    printf("\nOdebrano sygnal SIGINT\n");

    if (child_is_alive)   //probably the most brutal thing i've ever wrote
    {
        printf("\nKonczenie procesu potomnego\n");
        kill(child,SIGKILL);
    }
    printf("\nKonczenie procesu macierzystego\n");
    exit(0);
}

void handle_tstp(int sig_no)
{
    if (child_is_alive)
    {
        printf("\nOczekuje na: \nCTRL+Z - kontynuacja, albo\n");
        printf("CTRL+C - zakonczenie programu\n");
        printf("\nKonczenie procesu potomnego\n");
        kill(child,SIGKILL);
        child_is_alive=0;
    }
    else
    {
        printf("\nPowrot do dzialania\n");
        make_child();
    }

}


int main(void)
{

    make_child();

    signal(SIGINT,handle_int);
    signal(SIGTSTP,handle_tstp);

    while(1) pause();

}