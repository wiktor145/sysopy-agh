//
// Created by wicia on 22.03.19.
//
#define _XOPEN_SOURCE 500  //necessary

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>

int sigintend = 0;

int working = 1;
int end = 0;

void stop_proces(int n)
{
   working=0;
}

void start_proces(int n)
{
    working = 1;
}

void end_proces(int n)
{
    end = 1;
    working=0;

}



struct proces
{
    char plik_procesu[PATH_MAX];
    int pid;
    int dziala;
};

void commands(void)
{
    printf("\nMozliwe komendy:\n");
    printf("LIST - wypisanie procesow potomnych monitorujacych pliki\n");
    printf("STOP PID - zatrzymanie procesu o numerze PID\n");
    printf("STOP ALL - zatrzymanie wszystkich procesow\n");
    printf("START PID - wznowienie dzialania procesu o nr PID\n");
    printf("START ALL - wznowienie dzialania wszystkich procesow\n");
    printf("END - zakonczenie wszystkich procesow oraz programu\n\n");
}

void list(struct proces dzieci[],int children_number)
{
    for (int i=0; i<children_number; i++)
    {
        printf("\nPID: %d\n",dzieci[i].pid);
        printf("Monitorowany plik: %s\n",dzieci[i].plik_procesu);
        if (dzieci[i].dziala) printf("Dziala\n");
        else printf("Wstrzymany\n");
    }
}




size_t line_size (FILE *f)
{
    size_t size = 0;
    char c;
    while ((c= (char)fgetc(f)) != '\n')
    {
        if (c == EOF)
        {
         break;
        }
        size++;
    }
    if (c != EOF ) size++;

    fseek(f,-size,1);
    return size;
}


int memory_monitor (char * path, long interval_time)
{
    struct sigaction act1;
    act1.sa_handler = stop_proces;
    sigemptyset(&act1.sa_mask);
    act1.sa_flags = 0;
    struct sigaction act2;
    act2.sa_handler = start_proces;
    sigemptyset(&act2.sa_mask);
    act2.sa_flags = 0;
    struct sigaction act3;
    act3.sa_handler = end_proces;
    sigemptyset(&act3.sa_mask);
    act3.sa_flags = 0;


    sigaction(SIGUSR1,&act1,NULL);
    sigaction(SIGCONT,&act2,NULL);
    sigaction(SIGUSR2,&act3,NULL);

    int copies = 0;

    FILE *f = fopen(path,"r");
    if (!f)
    {
        free(path);
        return -1;
    }

    char* tmpfile;
    int filesize = 0;
    char c;
    while((c = fgetc(f)) != EOF) filesize++;
    rewind(f);
    tmpfile = malloc(filesize*sizeof(char));
    if (!tmpfile)
    {
        fclose(f);
        free(path);
        return -1;
    }
    int i=0;
    while((c = fgetc(f)) != EOF) tmpfile[i++]=c;
    rewind(f);
    fclose(f);

    struct stat buf;
    lstat(path,&buf);
    time_t filetime = buf.st_mtime;
    while (end == 0) {
        while (working) {
            if (lstat(path, &buf) == -1) // maybe fil was deleted?
            {
                free(path);
                free(tmpfile);
                return -1;
            }
            if (buf.st_mtime != filetime) // date of modification changed
            {
                copies++;
                char newfilename[PATH_MAX + 50];
                char timebuff[20];
                strftime(timebuff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&filetime));

                sprintf(newfilename, "./archiwum/%s_%s", basename(path), timebuff);
                FILE *kopia = fopen(newfilename, "a");
                if (!kopia) {
                    free(path);
                    free(tmpfile);
                    return -1;
                }

                if (filesize > 0) fwrite(tmpfile, sizeof(char), filesize, kopia);

                fclose(kopia);


                f = fopen(path, "r");
                if (!f) {
                    free(path);
                    return -1;
                }

                filetime = buf.st_mtime;

                free(tmpfile);
                filesize = 0;
                while ((c = fgetc(f)) != EOF) filesize++;
                rewind(f);
                tmpfile = malloc(filesize * sizeof(char));
                if (!tmpfile) {
                    fclose(f);
                    free(path);
                    return -1;
                }
                i = 0;
                while ((c = fgetc(f)) != EOF) tmpfile[i++] = c;
                rewind(f);
                fclose(f);
            }
            if (working && !end) sleep(interval_time);
        }
        if (!end)
        {
            pause();
        }
    }
    free(tmpfile);
    free(path);

    return copies;
}

void end_handler(int n)
{
    sigintend=1;
}


int main (int argc, char** argv)
{


    if (argc != 2)
    {
        printf("Wrong number of arguments, should be path to the file\n");
        exit(1);
    }

    FILE *f = fopen(argv[1],"r");

    if (!f)
    {
        printf("Error in opening file\n");
        exit(1);
    }

    struct proces dzieci[20];

        pid_t pid;
        int children_number = 0;
        int c;
        char * buff;

        while ((c = fgetc(f)) != EOF)
        {
            fseek(f, -1, 1); //return pointer
            size_t size = line_size(f);
            buff = malloc (size*sizeof(char)+1);
            if (!buff)
            {
                printf("error in alocating space\n");
                fclose(f);
                return 1;
            }
            buff[size]='\0';
            fread(buff,size,1,f);
            if (buff == NULL)
            {
                printf("some error\n");
                exit(1);
            }

            int pathsize = 0;
            for (;  pathsize<size && buff[pathsize]!='\n' && buff[pathsize]!= '\0' && buff[pathsize]!=' '; pathsize++);
            if (pathsize == size || buff[pathsize]=='\0')
            {
                free(buff);
                return -1;
            }
            char *path = malloc(pathsize*sizeof(char)+1);
            if (path == NULL)
            {
                free(buff);
                return -1;
            }
            path[pathsize]='\0';
            for (int i=0; i<pathsize; i++) path[i]=buff[i];

            int timesize =0;

            for (int i=pathsize+1; i<size && buff[i]!= '\n' && buff[i]!='\0'; i++, timesize++);

            char *fortime = malloc (timesize*sizeof(char)+1);
            fortime[timesize]='\0';
            for (int i=pathsize+1, j=0; i<size && buff[i]!='\n' && buff[i]!='\0'; i++, j++) fortime[j]=buff[i];

            long interval_time = strtol(fortime, NULL, 10);

            if (interval_time <= 0)
            {
                free(buff);
                free(fortime);
                free(path);
                fclose(f);
                return -1;
            }
            free(fortime);

            pid=fork();


            if (pid == 0)
            {
                if (buff != NULL) free(buff);
                int ret = memory_monitor(path, interval_time);
                fclose(f);
                exit(ret);
            }
            else if (pid == -1)
            {
                printf("Could not create child\n");
            }
            else
            {
                strcpy(dzieci[children_number].plik_procesu,path);
                dzieci[children_number].pid=pid;
                dzieci[children_number].dziala=1;
                children_number++;

            }

            if (buff != NULL) free(buff);

            free(path);

        }
        if (children_number == 0)
        {
            printf("Nie utworzono zadnych procesow\n");
            fclose(f);
            return 1;

        }
        list(dzieci,children_number);
        commands();
        char polecenie[40];
        char polecenie1[40];

        struct sigaction act1;
        act1.sa_handler = end_handler;
        sigemptyset(&act1.sa_mask);
        act1.sa_flags = 0;

        sigaction(SIGINT,&act1,NULL);

        while(1)
        {


        etykieta:
            polecenie[0]='\0';
            polecenie1[0]='\0';

            printf("Podaj polecenie do wykonania:\n");
            scanf("%s",polecenie);

            if (sigintend)
            {
                int status;

                for (int i=0; i<children_number; i++) kill(dzieci[i].pid,SIGUSR2);

                for (int i=0; i<children_number; i++)
                {

                    waitpid(dzieci[i].pid,&status,0);

                    if (WEXITSTATUS(status) != 255) printf("Proces %d utworzyl %d kopii pliku\n",dzieci[i].pid,WEXITSTATUS(status));
                    else
                    {
                        printf("Error in file\n");
                        //return 1;
                    }

                }

                rewind(f);
                fclose(f);
                return 0;

            }

            if (strcmp(polecenie,"LIST")==0)
            {
                list(dzieci,children_number);
            }
            else if (strcmp(polecenie,"STOP")==0)
            {
                scanf("%s",polecenie1);
                if (strcmp(polecenie1,"ALL")==0)
                {
                    for (int i=0; i<children_number; i++)
                    {
                        if (dzieci[i].dziala)
                        {
                            kill(dzieci[i].pid,SIGUSR1);
                            printf("Zatrzymano proces %d\n",dzieci[i].pid);
                            dzieci[i].dziala=0;
                        }
                        else printf("Proces %d jest juz zatrzymany\n",dzieci[i].pid);
                    }
                }
                else
                {
                    long pid_child = strtol(polecenie1,NULL,10);
                    if (!pid_child)
                    {
                        printf("Error in  PID number\n");
                        continue;
                    }
                    for (int i=0; i<children_number; i++)
                    {
                        if (dzieci[i].pid == pid_child)
                        {
                            if (dzieci[i].dziala)
                            {
                                kill(dzieci[i].pid,SIGUSR1);
                                printf("Zatrzymano proces %d\n",dzieci[i].pid);
                                dzieci[i].dziala=0;

                            }
                            else printf("Proces %d jest juz zatrzymany\n",dzieci[i].pid);
                            goto etykieta;
                        }
                    }
                    printf("Brak procesu o podanym PID\n");

                }


            }
            else if (strcmp(polecenie,"START")==0)
            {
                scanf("%s",polecenie1);
                if (strcmp(polecenie1,"ALL")==0)
                {
                    for (int i=0; i<children_number; i++)
                    {
                        if (!dzieci[i].dziala)
                        {
                            kill(dzieci[i].pid,SIGCONT);
                            printf("Wznowiono proces %d\n",dzieci[i].pid);
                            dzieci[i].dziala=1;

                        }
                        else printf("Proces %d juz dziala\n",dzieci[i].pid);
                    }
                }
                else
                {
                    long pid_child = strtol(polecenie1,NULL,10);
                    if (!pid_child)
                    {
                        printf("Error in  PID number\n");
                        continue;
                    }
                    for (int i=0; i<children_number; i++)
                    {
                        if (dzieci[i].pid == pid_child)
                        {
                            if (!dzieci[i].dziala)
                            {
                                kill(dzieci[i].pid,SIGCONT);
                                printf("Wznowionono proces %d\n",dzieci[i].pid);
                                dzieci[i].dziala=1;

                            }
                            else printf("Proces %d juz dziala\n",dzieci[i].pid);

                            goto etykieta;
                        }
                    }
                    printf("Brak procesu o podanym PID\n");

                }


            }
            else if (strcmp(polecenie,"END")==0)
            {


                int status;

                for (int i=0; i<children_number; i++) kill(dzieci[i].pid,SIGUSR2);

                for (int i=0; i<children_number; i++)
                {

                    waitpid(dzieci[i].pid,&status,0);
                    printf("%d\n",WEXITSTATUS(status));
                    if (WEXITSTATUS(status) != 255) printf("Proces %d utworzyl %d kopii pliku\n",dzieci[i].pid,WEXITSTATUS(status));
                    else
                    {
                        printf("Error in file\n");
                        //return 1;
                    }

                }

                rewind(f);
                fclose(f);
                return 0;
            }
            else
            {
                int c;
                while ((c = getchar()) != '\n' && c != EOF) { }
                printf("Nieznane polecenie\n");
                commands();
            }


        }

}