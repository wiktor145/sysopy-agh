//
// Created by wicia on 22.03.19.
//
#define _XOPEN_SOURCE 500  //necessary
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/resource.h>

int set_limit(long processor_time, long virtual_memory)
{
    struct rlimit proc;
    struct rlimit virt;

    getrlimit(RLIMIT_CPU,&proc);
    getrlimit(RLIMIT_AS,&virt);

    //printf("%lu %lu %lu %lu\n",proc.rlim_cur,proc.rlim_max, virt.rlim_cur, virt.rlim_max );

    if (processor_time > proc.rlim_max)
    {
        printf("Limit for processor is higher than given by kernel\n");
        return -1;
    }

    if (virtual_memory > virt.rlim_max)
    {
        printf("Limit for virtual memory is higher than given by kernel\n");
        return -1;
    }

    proc.rlim_max = processor_time;
    proc.rlim_cur = processor_time/2;

    virt.rlim_max = virtual_memory;
    virt.rlim_cur = virtual_memory/2;

    // printf("%lu %lu %lu %lu\n",proc.rlim_cur,proc.rlim_max, virt.rlim_cur, virt.rlim_max );


    if (setrlimit(RLIMIT_CPU,&proc) == -1)
    {
        printf("Error in setting cpu limit\n");

        return -1;
    }
    if (setrlimit(RLIMIT_AS,&virt) == -1)
    {
        printf("Error in setting memory limit\n");

        return -1;
    }

    return 0;
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


int memory_monitor (char * buff, int size, int monitoring_time)
{
    time_t start;
    time(&start); //for measuring time
    time_t end;
    int copies = 0;

    int pathsize = 0;
    for (;  pathsize<size && buff[pathsize]!='\n' && buff[pathsize]!= '\0' && buff[pathsize]!=' '; pathsize++);
    if (pathsize== size || buff[pathsize]=='\0')
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
        return -1;
    }


    FILE *f = fopen(path,"r");
    if (!f)
    {
        free(buff);
        free(fortime);
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
        free(buff);
        free(fortime);
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

    time(&end);
    while ((end-start) < monitoring_time)
    {
        //printf("%f \n",(double)(end-start));
        if (lstat(path,&buf) ==-1) // maybe fil was deleted?
        {
            free(buff);
            free(fortime);
            free(path);
            free(tmpfile);
            return -1;
        }
        if (buf.st_mtime!=filetime) // date of modification changed
        {
            copies++;
            char newfilename[PATH_MAX+50];
            char timebuff[20];
            strftime(timebuff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&filetime));

            sprintf(newfilename,"./archiwum/%s_%s",basename(path),timebuff);
            FILE *kopia = fopen(newfilename,"a");
            if (!kopia)
            {
                free(buff);

                free(fortime);
                free(path);
                free(tmpfile);
                return -1;
            }

            if (filesize > 0 ) fwrite(tmpfile,sizeof(char),filesize,kopia);

            fclose(kopia);


            f = fopen(path,"r");
            if (!f)
            {
                free(buff);
                free(fortime);
                free(path);
                return -1;
            }

            filetime = buf.st_mtime;

            free(tmpfile);
            filesize = 0;
            while((c = fgetc(f)) != EOF) filesize++;
            rewind(f);
            tmpfile = malloc(filesize*sizeof(char));
            if (!tmpfile)
            {
                fclose(f);
                free(buff);
                free(fortime);
                free(path);
                return -1;
            }
            i=0;
            while((c = fgetc(f)) != EOF) tmpfile[i++]=c;
            rewind(f);
            fclose(f);
        }
        sleep(interval_time);
        time(&end);
    }

    free(tmpfile);
    free(fortime);
    free(path);
    free(buff);

    return copies;
}

int non_memory_monitor (char * buff, int size, int monitoring_time,long processor_time, long virtual_memory)
{

    time_t start;
    time(&start); //for measuring time
    time_t end;
    char timebuff[20];

    int pathsize = 0;
    for (;  pathsize<size && buff[pathsize]!='\n' && buff[pathsize]!= '\0' && buff[pathsize]!=' '; pathsize++);
    if (pathsize== size || buff[pathsize]=='\0')
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
        return -1;
    }

    struct stat buf;
    lstat(path,&buf);
    time_t filetime = buf.st_mtime;


    char newfile[PATH_MAX+50];


    strftime(timebuff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&filetime));

    sprintf(newfile,"./archiwum/%s_%s",basename(path),timebuff);


    pid_t child_pid;
    child_pid = fork();
    if(child_pid==0) {
        execl("/usr/bin/cp","cp",path,newfile,NULL);
    }
    int copies = 1;


    time(&end);

    while ((end-start) < monitoring_time)
    {
       // printf("%f",(double)(end-start));
        if (lstat(path,&buf) ==-1) // maybe file was deleted?
        {
            free(buff);
            free(fortime);
            free(path);
            return -1;
        }
        if (buf.st_mtime!=filetime) // date of modification changed
        {
            copies++;
            char newfilename[PATH_MAX+50];

            filetime = buf.st_mtime;
            char timebuff[20];

            strftime(timebuff, 20, "%Y-%m-%d_%H-%M-%S", localtime(&filetime));

            sprintf(newfilename,"./archiwum/%s_%s",basename(path),timebuff);

            child_pid = fork();
            if(child_pid==0) {

                execl("/usr/bin/cp","cp",path,newfilename,NULL);
                printf("Error in exec\n");
                exit(-1);
            }
            else if (child_pid == -1)
            {
                printf("Could not create child\n");
            }

        }
        sleep(interval_time);
        time(&end);
    }

    free(fortime);
    free(path);
    free(buff);

    return copies;

}



int main (int argc, char** argv)
{
    if (argc != 6)
    {
        printf("Wrong number of arguments, should be path to the file\n");
        printf("Second should be monitoring time in seconds, third\n");
        printf("should be type of execution, fourth restriction on time in seconds\n");
        printf("And last, restriction on virtual memory in megabytes (just number)\n");
        exit(1);
    }

    FILE *f = fopen(argv[1],"r");

    if (!f)
    {
        printf("Error in opening file\n");
        exit(1);
    }

    long monitoring_time = strtol(argv[2],NULL,10);

    if (monitoring_time <=0)
    {
        printf("Error in time\n");
        fclose(f);
        exit(1);
    }

    long processor_time = strtol(argv[4],NULL, 10);

    if (processor_time <=0)
    {
        printf("Error in  processor time\n");
        fclose(f);
        exit(1);
    }

    long virtual_memory = strtol(argv[5],NULL, 10);

    if (virtual_memory <=0)
    {
        printf("Error in virtual memory\n");
        fclose(f);
        exit(1);
    }
    virtual_memory*=1048576; // converting mbytes to bytes


    if (strcmp(argv[3],"1\0") == 0)  //pierwszy tryb z kopiowaniem do pamieci
    {
        pid_t pid;
        int children_number = 0;
        int c;
        char * buff;
        struct rusage prev_usage;
        getrusage(RUSAGE_CHILDREN,& prev_usage);

        while ((c = fgetc(f)) != EOF)
        {
            fseek(f, -1, 1); //return pointer
            size_t linesize = line_size(f);
            buff = malloc (linesize*sizeof(char)+1);
            if (!buff)
            {
                printf("error in alocating space\n");
                fclose(f);
                return 1;
            }
            buff[linesize]='\0';
            fread(buff,linesize,1,f);
            if (buff == NULL)
            {
                printf("some error\n");
                exit(1);
            }

            children_number++;
            pid=fork();


            if (pid == 0)
            {
                if (set_limit(processor_time,virtual_memory) == -1)
                {
                    fclose(f);
                    exit(-1);
                }
                int ret = memory_monitor(buff, linesize, monitoring_time);
                fclose(f);
                exit(ret);
            }
            else if (pid == -1)
            {
                printf("Could not create child\n");
            }
            if (buff != NULL) free(buff);

        }
        int status;
        int child_pid;
        struct rusage usage;
        struct rusage calculated_time;

        for (int i=0; i<children_number; i++)
        {
            child_pid=waitpid(0,&status,0);
            getrusage(RUSAGE_CHILDREN,& usage);
            timersub(&usage.ru_utime,&prev_usage.ru_utime,&calculated_time.ru_utime);
            timersub(&usage.ru_stime,&prev_usage.ru_stime,&calculated_time.ru_stime);

            if (WEXITSTATUS(status) != 255)
            {
                printf("Proces %d utworzyl %d kopii pliku\n",child_pid,WEXITSTATUS(status));
                printf("Uzyl %f czasu systemowego oraz\n",(double)(calculated_time.ru_stime.tv_sec*1000000+calculated_time.ru_stime.tv_usec)/1000000);
                printf("%f czasu uzytkownika\n\n",(double)(calculated_time.ru_utime.tv_sec*1000000+calculated_time.ru_utime.tv_usec)/1000000);


            }
            else
            {
                printf("Error in file\n");
                // return 1;
            }
            prev_usage=usage;

        }

    }
    else if (strcmp(argv[3],"2\0") == 0) // drugi tryb bez k
    {
        pid_t pid;
        int children_number = 0;
        int c;
        char * buff;
        struct rusage prev_usage;
        getrusage(RUSAGE_CHILDREN,& prev_usage);

        while ((c = fgetc(f)) != EOF)
        {
            fseek(f, -1, 1); //return pointer
            size_t linesize = line_size(f);
            buff = malloc (linesize*sizeof(char)+1);
            if (!buff)
            {
                printf("error in alocating space\n");
                fclose(f);
                return 1;
            }
            buff[linesize]='\0';
            fread(buff,linesize,1,f);
            if (buff == NULL)
            {
                printf("some error\n");
                exit(1);
            }

            children_number++;
            pid=fork();

            if (pid == 0)
            {
                if (set_limit(processor_time,virtual_memory) == -1)
                {
                    fclose(f);
                    exit(-1);
                }
                int status = non_memory_monitor(buff, linesize, monitoring_time,processor_time,virtual_memory);
                fclose(f);
                exit(status);
            }
            else if (pid == -1)
            {
                printf("Could not create child\n");
            }
            if (buff != NULL) free(buff);

        }
        int status=0;
        int child_pid;
        struct rusage usage;
        struct rusage calculated_time;

        for (int i=0; i<children_number; i++)
        {
            child_pid=waitpid(0,&status,0);
            getrusage(RUSAGE_CHILDREN,& usage);
            timersub(&usage.ru_utime,&prev_usage.ru_utime,&calculated_time.ru_utime);
            timersub(&usage.ru_stime,&prev_usage.ru_stime,&calculated_time.ru_stime);

            if (WEXITSTATUS(status) != 255)
            {
                printf("Proces %d utworzyl %d kopii pliku\n",child_pid,WEXITSTATUS(status));
                printf("Uzyl %f czasu systemowego oraz\n",(double)(calculated_time.ru_stime.tv_sec*1000000+calculated_time.ru_stime.tv_usec)/1000000);
                printf("%f czasu uzytkownika\n\n",(double)(calculated_time.ru_utime.tv_sec*1000000+calculated_time.ru_utime.tv_usec)/1000000);


            }
            else
            {
                printf("Error in file\n");
               // return 1;
            }
            prev_usage=usage;

        }

    }
    else
    {
        printf("Wrong third argument, should be 1 (with copying to memory) or 2\n");
        exit(1);
    }

    rewind(f);
    fclose(f);

    return 0;
}