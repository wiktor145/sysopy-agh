//
// Created by wicia on 21.03.19.
//
#define _XOPEN_SOURCE 500  //necessary
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ftw.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <sys/wait.h>

int ends_with_slash( char * path)
{
    int length = strlen(path);
    if (length > 0 && path[length-1] == '/') return 1;
    else return 0;

}


void stat_search(char *path)
{
    DIR *dir =opendir(path);
    if (!dir && errno != 0)
    {
        //printf("Error in opening directory %s, errno : %d\n",path,errno);  // <- if you are searching / or /proc or something like this
        //do not uncomment this line, or you will get plenty of errno 13 (permission denied) messages
        return;
    }
    char buffer[PATH_MAX+10];
    sprintf(buffer,"ls -l %s",path);

    printf("%s\n", path);
    printf("PID: %d\n",(int)getpid());
    system(buffer);
    printf("\n");

    struct dirent* file;
    struct stat buf;
    pid_t child_pid; // for distinguishing between parent and child

    file=readdir(dir);
    while (file)
    {

        if (strcmp(file->d_name,".\0") != 0 && strcmp(file->d_name,"..\0") != 0)
            {
                char path1[PATH_MAX];
                strcpy(path1,path);
                if (strcmp(path1,"/\0") != 0 && !ends_with_slash(path)) strcat(path1,"/");
                strcat(path1,file->d_name);
                lstat(path1,&buf);

                if ((buf.st_mode & S_IFMT) == S_IFDIR && (buf.st_mode & S_IFMT) != S_IFLNK)
                {
                    child_pid=fork();
                    if (child_pid==0)  // we are in child process, we stop looking through this dir
                    {
                        closedir(dir);
                        stat_search(path1);
                        exit(0);
                    }
                    wait(NULL); // wait for child
                }
            }
        file=readdir(dir);
    }
    closedir(dir);
}


int main (int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Invalid number of arguments. you should only give directory path in \"\" \n");
        printf("Exiting...\n");
        return 1;
    }

    char *path = argv[1];

    stat_search(path);

    return 0;
}