//
// Created by wicia on 03.04.19.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int valid_line(char* line, int n)
{
    if (line[0] == '|' || line[0] == '\n' || line[0] == '\0') return 0; //pusta linia
    //albo zaczynajaca sie od pipe'a
    int size = 0;
    int i=1;

    while (line[size] != '\0' && line[size] != '\n')
    {
        size++;
    }

    while (line[size-i] == ' ' || line[size-i] == '\t')
    {

        if (i < size)i++;
        else return 0;
    }
    if (line[size-i] == '|') return 0; // czy na koncu nie ma samotnej rury

    int pipes = 0;
    i=0;
    int now_pipe = 0;
    //int now_arg = 1;

    for(; i<size; i++)
    {
        if (line[i] == '|')
        {
            if (!now_pipe) return 0;
            else
            {
                pipes++;
                if (pipes == 5) return 0;
                now_pipe=0;
            }
        }
        else
        {
            if (line[i] != ' ' && line[i] != '\t') now_pipe=1;
        }

    }

    return (pipes<=4) ? 1 : 0;

}

void parse(char line[], char arg1[], int* line_it)
{
    int arg_it = 0;

    while (line[*line_it] != '|' && line[*line_it] != '\n' && line[*line_it] != '\0')
    {
        arg1[arg_it] = line[*line_it];
        (*line_it)++;
        arg_it++;
    }

    if (arg_it > 200)
    {
        printf("To long argument\n");
        exit(1);
    }
    arg1[arg_it] = '\0';

}

void parse_args(char arg[200], char* args[7])
{
    int argument = 0;
    int i=0;
    int j=0;

    while (i< 200 && arg[i]!='\n' && arg[i]!='\0')
    {

        while (arg[i] == ' ' || arg[i] == '\t') i++;

        if (arg[i]=='\n' || arg[i]=='\0') break;

        j=0;
        args[argument] = malloc(40*sizeof(char));

        while (i< 200 && arg[i]!='\n' && arg[i]!='\0' && arg[i] != ' ' && arg[i] != '\t')
        {
            args[argument][j]=arg[i];
            i++;
            j++;
        }
        args[argument++][j] = '\0';

        i++;
    }

    args[argument] = NULL;

}


int main(int argc, char**argv)
{
    if (argc != 2)
    {
        printf("Nie podano sciezki do pliku!\n");
        exit(1);
    }

    FILE* f = fopen(argv[1],"r");

    if (!f)
    {
        printf("Blad w trakcie otwierania pliku!\n");
        exit(1);
    }

    char line[1000]; // for all line
    char arg1[200];
    arg1[0]='\0';
    char arg2[200];
    arg2[0]='\0';

    char arg3[200];
    arg3[0]='\0';

    char arg4[200];
    arg4[0]='\0';

    char arg5[200];
    arg5[0]='\0';

    // for all executed commands
    char* args[5][7];
    for (int a=0;a<5; a++)
        for (int b=0; b<7; b++)
            args[a][b]=NULL;

    int pd[4][2];

    int line_it, arguments;

    while (fgets(line,1000,f) != NULL)
    {
        if (!valid_line(line,1000))
        {
            printf("Error in line %s\n",line);
            printf("Ignoring\n");
            //fclose(f);
            continue;
        }

        for (int a=0;a<5; a++)
            for (int b=0; b<6; b++)
            {
                if (args[a][b] != NULL) free(args[a][b]);
                args[a][b]=NULL;
            }

        line_it=0;
        arguments = 1;

        parse(line,arg1,&line_it);
        parse_args(arg1,args[0]);
        if (line[line_it] == '|') //one more
        {
            arguments++;
            line_it++;
            parse(line,arg2,&line_it);

            parse_args(arg2,args[1]);

            if (line[line_it] == '|') //one more
            {
                arguments++;
                line_it++;

                parse(line,arg3,&line_it);
                parse_args(arg3,args[2]);

                if (line[line_it] == '|') //one more
                {
                    arguments++;
                    line_it++;

                    parse(line,arg4,&line_it);
                    parse_args(arg4,args[3]);

                    if (line[line_it] == '|') //one more
                    {
                        arguments++;
                        line_it++;

                        parse(line,arg5,&line_it);
                        parse_args(arg5,args[4]);

                    }
                }
            }
        }

        for(int i=0; i<arguments -1; i++) pipe(pd[i]);

        for (int i = 0; i < arguments; i++)
        {
            int pid = fork();
            if (pid == 0)
            {

                if (i > 0)
                    if (dup2(pd[i - 1][0], STDIN_FILENO) < 0)
                    {
                        printf("Error in dup2 function!\n");
                        exit(1);
                    }

                if (i < arguments - 1) {

                    if (dup2(pd[i][1], STDOUT_FILENO) < 0)
                    {
                        printf("Error in dup2 function!\n");
                        exit(1);
                    }
                }
                for(int i=0; i<arguments -1; i++)
                {
                    close(pd[i][0]);
                    close(pd[i][1]);
                }

                execvp(args[i][0], args[i]);
                exit(1);
            }
//
        }

        for(int i=0; i<arguments -1; i++)
        {
            close(pd[i][0]);
            close(pd[i][1]);
        }

       while (wait(NULL) > 0){}

       printf("\n\n\n");

    }

    for (int i=0; i<6; i++)
    {
        if (args[0][i] != NULL) free(args[0][i]);
        if (args[1][i] != NULL) free(args[1][i]);
        if (args[2][i] != NULL) free(args[2][i]);
        if (args[3][i] != NULL) free(args[3][i]);
        if (args[4][i] != NULL) free(args[4][i]);
    }
    fclose(f);


    return 0;
}

