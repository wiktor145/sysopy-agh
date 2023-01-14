//
// Created by wicia on 09.03.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/times.h>
#include <unistd.h>  // to get CLK TCK
#include "library.h"

void test (void);

void print_times(FILE *f, clock_t real, struct tms *tmstart, struct tms *tmstop, char * message);




/*
 * Possible arguments are:
 * create_table size - creates table  (it must be the first task to do)
 * it can be executed only once
 * search_directory dir file file_name_temp - finds file "file" in "dir" directory
 * and saves the outcome to the array
 * remove_block index  - removes block at index "index"
 * add_block - adds content of temp file to array block and returns block index
 */





int main (int argc, char * argv[])
{
    if (argc == 1 )  // if program was executed without arguments, testing procedure starts
    {
        test();
        return 0;
    }
    if (argc == 2)
    {
        printf("Not enought arguments, 2 is minimal (create_table size ...)\n");
        exit(1);
    }
    else
    {
        //if first arg is not create_table, exit
        int argindex = 1; // actual index in arguments array

        if ((strcmp(argv[argindex],"create_table\0")) != 0)
        {
            printf("First task to do must be create_table. Exiting\n");
            exit(1);
        }

        int size = (int) strtol(argv[++argindex],NULL,10);

        if (size == 0)
        {
            printf("Provided table size is invalid, exiting...\n");
            exit(1);
        }

        Array * array = create_table(size);

        argindex++;

        while (argindex < argc)
        {
            if (strcmp(argv[argindex],"create_table\0") == 0)
            {
                printf("You can create table only once, exiting\n");
                free_all(array);
                free(array);
                exit(1);
            }

            else if (strcmp(argv[argindex],"remove_block\0") == 0)
            {
                argindex++;
                if (argindex == argc)
                {
                    printf("Not enough arguments, exiting\n");
                    free_all(array);
                    free(array);
                    exit (1);
                }

                char *endptr;

                int index = strtol(argv[argindex++], &endptr, 10);

                if (endptr == argv[argindex-1])
                {
                    printf("argument %s is not a proper index value, exiting\n",argv[argindex-1]);
                    free_all(array);
                    free(array);
                    exit(1);
                }

                remove_block(array, index);
                printf("Removed block from index %d\n",index);

            }

            else if (strcmp(argv[argindex],"add_block\0") == 0)
            {
                argindex++;

                int index = reserve_block(array);
                printf("Reserved and added block at index %d\n", index);

            }

            else if (strcmp(argv[argindex],"search_directory\0") == 0)
            {
                argindex++;

                if (argindex + 3 > argc )
                {
                    printf("Not enough arguments, exiting\n");

                    free_all(array);
                    free(array);
                    exit (1);
                }

                set_directory_to_search(argv[argindex++]);
                set_file_to_find(argv[argindex++]);
                set_tmp_file(argv[argindex++]);
                find_file_and_copy_to_temp();
                printf("Searching %s %s %s completed\n", argv[argindex-3],argv[argindex-2],argv[argindex-1]);

            }

            else
            {
                printf("Wrong argument %s  exiting\n",argv[argindex]);
                free_all(array);
                free(array);
                exit(1);
            }

        }
        free_all(array);
        free(array);
    }

    return 0;

}


void test (void)   // tests library and writes times to file "raport2.txt'
{
    printf("Testing procedure begins...\n\n");

    struct Array * array = create_table(5);

    FILE *f = fopen("raport2.txt","a");
    if (f == NULL)
    {
        printf("Error in opening file, exiting\n");
        free_all(array);
        free(array);
        exit(1);
    }

    int index;
    struct tms tmstart,tmstop;   //necessary for measuring time
    clock_t start, end;

//    struct tms{
//        clock_t tms_utime;   // user time
//        clock_t tms_stime;   // system time
//       ...
//    };

//testing for large searching

    set_directory_to_search("/");
    set_file_to_find("*.spec");
    set_tmp_file("tmpfile");

    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    find_file_and_copy_to_temp();
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for large searching");

// testing for medium searching
    set_directory_to_search("/home");
    set_file_to_find("library.h");


    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    find_file_and_copy_to_temp();
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for medium searching");


// testing for small searching
    set_directory_to_search("/home/wicia/CLionProjects/sysopy");

    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    find_file_and_copy_to_temp();
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for small searching");


// testing small saving and deleting
    set_tmp_file("small.txt");
    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    index=reserve_block(array);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for small saving");


    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    remove_block(array,index);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for small deleting");



// testing medium saving and deleting
    set_tmp_file("medium.txt");
    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    index=reserve_block(array);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for medium saving");


    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    remove_block(array,index);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for medium deleting");



// testing big saving and deleting
    set_tmp_file("big.txt");
    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    index=reserve_block(array);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for large saving");


    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    remove_block(array,index);
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for large deleting");


//testing adding and removing 1000 times big file

    set_tmp_file("big.txt");
    if ((start = times(&tmstart)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }
    for (int i=0; i<1000; i++)
    {
        index=reserve_block(array);
        remove_block(array,index);
    }
    if ((end = times(&tmstop)) == -1 )
    {
        printf("Error in measuring time\n");
        free_all(array);
        free(array);
        exit(1);
    }

    print_times(f,end-start,&tmstart,&tmstop, "Times for 1000 big saves an deletings");


    free_all(array);
    free(array);
    fclose(f);
    printf("End of testing procedure\n");
}

void print_times(FILE *f, clock_t real, struct tms *tmstart, struct tms *tmstop, char * message)
{
    long clktck;  //system clock ticks per second

    if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
    {
        printf("System error\n");
        free_all(NULL);
        exit(1);
    }

    fprintf(f,"%s",message);
    fprintf(f,"\n");
    fprintf(f,"real time: %7.2f\n", real/(double) clktck);
    fprintf(f,"user time: %7.2f\n", (tmstop->tms_utime - tmstart->tms_utime)/(double) clktck);
    fprintf(f,"system time: %7.2f\n", (tmstop->tms_stime - tmstart->tms_stime)/(double) clktck);
    fprintf(f,"-----------------\n\n\n");

    printf("%s",message);
    printf("\n");
    printf("real time: %7.2f\n", real/(double) clktck);
    printf("user time: %7.2f\n", (tmstop->tms_utime - tmstart->tms_utime)/(double) clktck);
    printf("system time: %7.2f\n", (tmstop->tms_stime - tmstart->tms_stime)/(double) clktck);
    printf("-----------------\n\n\n");

}