//
// Created by wicia on 18.03.19.
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

enum timecomparator {LESS, EQUAL, MORE};  // for comparison -> LESS = < ; EQUAL = = ; MORE = >


time_t globaldate;   //global variables for easier life
enum timecomparator globalsign;


void showinfo(const char *filepath, const struct stat *file)
{
    printf("%s \n",filepath);
    switch (file->st_mode & S_IFMT) {     // for checking file type
        case S_IFBLK:
            printf("block dev\n");
            break;
        case S_IFCHR:
            printf("char dev\n");
            break;
        case S_IFDIR:
            printf("dir\n");
            break;
        case S_IFIFO:
            printf("FIFO\n");
            break;
        case S_IFLNK:
            printf("slink\n");
            break;
        case S_IFREG:
            printf("file\n");
            break;
        case S_IFSOCK:
            printf("sock\n");
            break;
    }
    printf("Size - %ld\n",file->st_size);
    printf("Last access - %s",ctime(&file->st_atime));  // ctime converts time_t to string format date
    printf("Last modified - %s\n",ctime(&file->st_mtime));

}

#ifdef STAT
void stat_search(char *path)
{
    DIR *dir =opendir(path);
    if (!dir && errno != 0)
    {
        //printf("Error in opening directory %s, errno : %d\n",path,errno);  // <- if you are searching / or /proc os something like this
        //do not uncomment this line, or you will get plenty of errno 13 (perrmision denied) messages
        return;
    }

    struct dirent* file;// = readdir(dir);
    struct stat buf;
    //char *path1 = NULL;
    file=readdir(dir);
    while (file)
    {

        if (strcmp(file->d_name,".\0") != 0 && strcmp(file->d_name,"..\0") != 0)
            {
                 char path1[PATH_MAX];
                 //path1=malloc(sizeof(path)+sizeof(file->d_name)+2);
                strcpy(path1,path);
                if (strcmp(path1,"/\0") != 0) strcat(path1,"/");
                strcat(path1,file->d_name);
                lstat(path1,&buf);
                int date_compare_result = difftime(globaldate,buf.st_mtime);

                if (date_compare_result < 0 && globalsign == MORE) showinfo(path1,&buf);
                if (date_compare_result == 0 && globalsign == EQUAL) showinfo(path1,&buf);
                if (date_compare_result > 0 && globalsign == LESS) showinfo(path1,&buf);


                if ((buf.st_mode & S_IFMT) == S_IFDIR && (buf.st_mode & S_IFMT) != S_IFLNK)
                {
                    stat_search(path1);
                }
            }
        file=readdir(dir);
    }
    closedir(dir);
}
#endif

#ifndef STAT
static int nftw_search(const char *filepath, const struct stat *file, int flag, struct FTW *ftwbuf)
{
    int date_compare_result = difftime(globaldate,file->st_mtime);

    if (date_compare_result < 0 && globalsign != MORE) return 0;
    if (date_compare_result == 0 && globalsign != EQUAL) return 0;  // time is not ok
    if (date_compare_result > 0 && globalsign != LESS) return 0;

    showinfo(filepath,file);
    return 0;
}
#endif

int main (int argc, char** argv)
{
    if (argc != 4)
    {
        printf("Invalid number of arguments. First should be directory, second < > or =\n and third date in format \"YYYY-MM-DD HH:MM:SS\"\n");
        printf("Exiting...\n");
        return 1;
    }

    if (strcmp(argv[2],">\0")==0)
    {
        globalsign = MORE;
    }
    else if (strcmp(argv[2],"=\0")==0)
    {
        globalsign = EQUAL;
    }
    else if (strcmp(argv[2],"<\0")==0)
    {
        globalsign = LESS;
    }
    else
    {
        printf("Invalid sign of date comparison\n");
        return 1;
    }

    char *path = argv[1];

    const char format[] = "%Y-%m-%d %H:%M:%S";  //date format

    struct tm *time = calloc(1,sizeof(struct tm)); // struct for time
    strptime(argv[3], format, time);  //converts string date do tm struct date
    globaldate = mktime(time);  //converts date to time_t
    //in struct stat times are in time_t format so it will be easier
    // to compare them
    free(time);
    char *rpath = realpath(path,NULL); //realpath returns absolute path

    #ifdef STAT
    stat_search(rpath);
    #endif

    #ifndef STAT

    nftw(rpath,nftw_search,20, FTW_PHYS);
    //20 is max descriptors opened at the same time
    //FW_PHYS means do not follow symbolic links
    #endif

    free(rpath);
    return 0;
}