#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>

void generate(char **argv);
void sort_lib(char **argv);
void sort_sys(char **argv);
void copy_lib(char **argv);
void copy_sys(char **argv);
void test(void);

int main(int argc, char **argv)
{
    if (argc == 1) test();  //executed without arguments, testing procedure starts
    else
    {
        if (strcmp(argv[1],"generate") == 0)
        {
            if (argc != 5)
            {
                printf("Not enough arguments\n");
                return 1;
            }

            generate(argv);
        }

        else if (strcmp(argv[1],"sort") == 0)
        {
            if (argc != 6 )
            {
                printf("Not enough arguments\n");
                return 1;
            }

            if (strcmp(argv[5],"lib") == 0)
                sort_lib(argv);
            else if (strcmp(argv[5],"sys") == 0)
                sort_sys(argv);
            else
            {
                printf("Last argument for sort function must be either sys or lib, exiting\n");
                return 1;
            }
        }

        else if (strcmp(argv[1],"copy") == 0)
        {
            if (argc != 7 )
            {
                printf("Not enough arguments\n");
                return 1;
            }

            if (strcmp(argv[6],"lib") == 0)
                copy_lib(argv);
            else if (strcmp(argv[6],"sys") == 0)
                copy_sys(argv);
            else
            {
                printf("Last argument for copy function must be either sys or lib, exiting\n");
                return 1;
            }
        }

        else
        {
            printf("Wrong function name %s - exiting\n",argv[1]);
            return 1;
        }
    }

    return 0;
}

void generate (char **argv)
{
    srand(time(NULL));
    int amount = strtol(argv[3],NULL,10);
    int size = strtol(argv[4],NULL,10);

    if (amount < 1)
    {
        printf("Error in amount %s, exiting\n",argv[3]);
        exit(1);
    }
    if (size < 1)
    {
        printf("Error in size %s, exiting\n",argv[4]);
        exit(1);
    }

    FILE * f = fopen(argv[2],"w+");
    if (!f)
    {
        printf("Error in creating file %s - exiting\n",argv[2]);
        exit(1);
    }
    char c;

    for (int a=0; a<amount; a++)  //amount times
        for (int b=0; b<size; b++)  //size times
        {
            c = (rand()%94)+33;  //draws char from ! to ~ (33-126)
            if (fwrite(&c,1,1,f) != 1)   //writes this char to file
            {
                printf("Error in writing to file, exiting\n");
                fclose(f);
                exit(1);
            }
        }

    fclose(f);
}

void sort_lib(char **argv)
{
    int amount = strtol(argv[3],NULL,10);
    int size = strtol(argv[4],NULL,10);
    if (amount < 1)
    {
        printf("Error in amount %s, exiting\n",argv[3]);
        exit(1);
    }
    if ( size < 1)
    {
        printf("Error in size %s, exiting\n",argv[4]);
        exit(1);
    }

    FILE *f = fopen(argv[2],"r+");
    if (f == NULL)
    {
        printf("Error in opening file %s, file might not exist, exiting...\n",argv[2]);
        exit(1);
    }

    char *a = calloc(size, size*sizeof(char));
    if (a == NULL)
    {
        printf("Error in allocating memory, exiting\n");
        fclose(f);
    }
    char *b = calloc(size, size*sizeof(char));
    if (b == NULL)
    {
        printf("Error in allocating memory, exiting\n");
        free(a);
        fclose(f);
    }

    long int off = (long int) (size * sizeof(char)); //for moving through file

    for (int j=0; j<amount; j++)        // bubble sort
    {
        rewind(f);
        for (int i=0; i<amount-1; i++)
        {
            fseek(f,i*off,SEEK_SET);  // get to proper place
            if(fread(a, sizeof(char), (size_t) size, f) != size)  // get first record
            {
                printf("Error in reading file\n");
                if (a!= NULL) free(a);
                if (b!= NULL) free(b);
                rewind(f);
                fclose(f);
                exit(1);
            }
            if(fread(b, sizeof(char), (size_t) size, f) != size)  // get second record
            {
                printf("Error in reading file\n");
                if (a!= NULL) free(a);
                if (b!= NULL) free(b);
                rewind(f);
                fclose(f);
                exit(1);
            }
            if (a[0]>b[0])  // if they are a wrong places
            {
                fseek(f,i*off,SEEK_SET);   // write second in first
                if(fwrite(b, sizeof(char), (size_t) size, f) != size)
                {
                    printf("Error in reading file\n");
                    if (a!= NULL) free(a);
                    if (b!= NULL) free(b);
                    rewind(f);
                    fclose(f);
                    exit(1);
                }
                if(fwrite(a, sizeof(char), (size_t) size, f) != size)  // write first in second
                {
                    printf("Error in reading file\n");
                    if (a!= NULL) free(a);
                    if (b!= NULL) free(b);
                    rewind(f);
                    fclose(f);
                    exit(1);
                }
            }
        }
    }
    rewind(f);

    if (a!= NULL) free(a);
    if (b!= NULL) free(b);
    fclose(f);


}

void sort_sys(char **argv)
{
    int amount = strtol(argv[3],NULL,10);
    int size = strtol(argv[4],NULL,10);
    if (amount < 1)
    {
        printf("Error in amount %s, exiting\n",argv[3]);
        exit(1);
    }
    if (size < 1)
    {
        printf("Error in size %s, exiting\n",argv[4]);
        exit(1);
    }

    int f = open(argv[2],O_RDWR);  //opens first file (descriptor) with read only
    if (f<0)
    {
        printf("error in opening file %s, exiting\n",argv[2]);
        exit(1);
    }

    char *a = calloc(size, size*sizeof(char));
    if (a == NULL)
    {
        printf("Error in allocating memory, exiting\n");
        close(f);
    }
    char *b = calloc(size, size*sizeof(char));
    if (b == NULL)
    {
        printf("Error in allocating memory, exiting\n");
        free(a);
        close(f);
    }

    long int off = (long int) (size * sizeof(char)); //for moving through file

    for (int j=0; j<amount; j++)        // bubble sort
    {
        lseek(f,0,SEEK_SET);
        for (int i=0; i<amount-1; i++)
        {
            lseek(f,(off_t)(i*off),SEEK_SET);  // get to proper place
            if(read(f,a,(size_t)size*sizeof(char)) != size)  // get first record
            {
                printf("Error in reading file\n");
                if (a!= NULL) free(a);
                if (b!= NULL) free(b);
                close(f);
                exit(1);
            }
            if(read(f,b,(size_t)size*sizeof(char)) != size)  // get second record
            {
                printf("Error in reading file\n");
                if (a!= NULL) free(a);
                if (b!= NULL) free(b);
                close(f);
                exit(1);
            }
            if (a[0]>b[0])  // if they are a wrong places
            {
                lseek(f,(off_t)(i*off),SEEK_SET);   // write second in first
                if(write(f,b,(size_t)size*sizeof(char)) != size)
                {
                    printf("Error in reading file\n");
                    if (a!= NULL) free(a);
                    if (b!= NULL) free(b);
                    close(f);
                    exit(1);
                }
                if(write(f,a,(size_t)size*sizeof(char)) != size)  // write first in second
                {
                    printf("Error in reading file\n");
                    if (a!= NULL) free(a);
                    if (b!= NULL) free(b);
                    close(f);
                    exit(1);
                }
            }
        }
    }
    lseek(f,0,SEEK_SET);
    if (a!= NULL) free(a);
    if (b!= NULL) free(b);
    close(f);
}

void copy_lib(char **argv)
{
    int amount = strtol(argv[4],NULL,10);
    int size = strtol(argv[5],NULL,10);
    if (amount < 1)
    {
        printf("Error in amount %s, exiting\n",argv[4]);
        exit(1);
    }
    if (size < 1)
    {
        printf("Error in size %s, exiting\n",argv[5]);
        exit(1);
    }

    FILE *f1 = fopen(argv[2],"r");
    if (f1 == NULL)
    {
        printf("Error in opening file %s, file might not exist, exiting...\n",argv[2]);
        exit(1);
    }

    FILE *f2 = fopen(argv[3],"w");
    if (f2 == NULL)
    {
        printf("Error in opening file %s, exiting...\n",argv[3]);
        fclose(f1);
        exit(1);
    }

    char * buffer = calloc(size,sizeof(char));
    if (buffer == NULL)
    {
        printf("Error in creating buffer, exiting...");
        fclose(f1);
        fclose(f2);
        exit(1);
    }

    for (int i=0; i<amount; i++)
    {
        int n;

        if (feof(f1)) break;

        if ((n=fread(buffer,sizeof(char),(size_t)(size),f1))<(size))
        {
            if (!feof(f1))
            {
              printf("Error in reading file, exiting...");
              if (buffer!=NULL) free(buffer);
              fclose(f1);
              fclose(f2);
              exit(1);
            }
            else
            {
               if (fwrite(buffer,sizeof(char),(size_t)n,f2)<n)
               {
                   printf("Error in writing to file, exiting...");
                   if (buffer!=NULL) free(buffer);
                   fclose(f1);
                   fclose(f2);
                   exit(1);
               }
               else
               {
                   printf("First file was smaller than expected");
                   break;
               }
            }
        }
        else
        {
            if (fwrite(buffer,sizeof(char),(size_t)n,f2)<n)
            {
                printf("Error in writing to file, exiting...");
                if (buffer!=NULL) free(buffer);
                fclose(f1);
                fclose(f2);
                exit(1);
            }
        }
    }

    if (buffer!=NULL) free(buffer);
    fclose(f1);
    fclose(f2);

}

void copy_sys(char **argv)
{
    int amount = strtol(argv[4],NULL,10);
    int size = strtol(argv[5],NULL,10);
    if ( amount < 1)
    {
        printf("Error in amount %s, exiting\n",argv[4]);
        exit(1);
    }
    if (size < 1)
    {
        printf("Error in size %s, exiting\n",argv[5]);
        exit(1);
    }

    int f1 = open(argv[2],O_RDONLY);  //opens first file (descriptor) with read only
    if (f1<0)
    {
        printf("error in opening file %s, exiting",argv[2]);
        exit(1);
    }

    int f2 = open(argv[3],O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    //opens second file (descriptor) for write only, if not exist create, if exist truncates,
    // gives read and write permissions for user
    if (f2 < 0)
    {
        printf("Error in opening file %s, exiting",argv[3]);
        close(f1);
        exit(1);
    }

    char * buffer = calloc(size,sizeof(char));
    if (buffer == NULL)
    {
        printf("Error in creating buffer, exiting...");
        close(f1);
        close(f2);
        exit(1);
    }

    for (int i=0; i<amount; i++)
    {
        int n;

        if ((n=read(f1,buffer,(size_t)size*sizeof(char)))<size)
        {
            if (n == 0)
            {
                printf("First file was smaller than expected");
                break;
            }
            if (n<0)
            {
                printf("Error in reading file, exiting...");
                if (buffer!=NULL) free(buffer);
                close(f1);
                close(f2);
                exit(1);
            }
            else
            {
                if (write(f2,buffer,(size_t)n*sizeof(char))<n)
                {
                    printf("Error in writing to file, exiting...");
                    if (buffer!=NULL) free(buffer);
                    close(f1);
                    close(f2);
                    exit(1);
                }
                else
                {
                    printf("First file was smaller than expected");
                    break;
                }
            }
        }
        else
        {
            if (write(f2,buffer,(size_t)n*sizeof(char))<n)
            {
                printf("Error in writing to file, exiting...");
                if (buffer!=NULL) free(buffer);
                close(f1);
                close(f2);
                exit(1);
            }
        }
    }

    if (buffer!=NULL) free(buffer);
    close(f1);
    close(f2);

}


void test(void)
{
    FILE * wynik = fopen("wyniki.txt","w");
    if (!wynik)
    {
        printf("Error\n");
        exit(1);
    }

    long clktck;  //system clock ticks per second

    if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
    {
        printf("System error\n");
        fclose(wynik);
        exit(1);
    }

    struct tms tmstart,tmstop;   //necessary for measuring time
    clock_t start, end;
    char** arg = calloc(7,sizeof(char*));
    char *plik = "plik\0", *tmp = "tmp\0";
    char **buffsize=calloc(6,sizeof(char*));
    buffsize[0]="1\0";
    buffsize[1]="4\0";
    buffsize[2]="512\0";
    buffsize[3]="1024\0";
    buffsize[4]="4096\0";
    buffsize[5]="8192\0";
    char**elements = calloc(12,sizeof(char*));
    elements[0]="2048\0";
    elements[1]="4096\0";

    elements[2]="2048\0";
    elements[3]="4096\0";

    elements[4]="2048\0";
    elements[5]="4096\0";

    elements[6]="2048\0";
    elements[7]="4096\0";

    elements[8]="2048\0";
    elements[9]="4096\0";

    elements[10]="2048\0";
    elements[11]="4096\0";

    fprintf(wynik,"#######sorting test######\n\n");
    for (int i=0; i<24; i++)  // sort testing
    {
        if(i%2==0)  // generating file
        {
            arg[2]=tmp;
            arg[4]=buffsize[i/4];
            arg[3]=elements[i/2];
            generate(arg);
        }

        arg[2]=tmp;
        arg[3]=plik;
        arg[5]=buffsize[i/4];    // copying from tmp to plik
        arg[4]=elements[i/2];
        copy_lib(arg);
        arg[2]=plik;
        arg[3]=elements[i/2];
        arg[4]=buffsize[i/4];

        if (i%2==0) {   // sys testing

            fprintf(wynik,"sys, buffer - %s, elements - %s\n",buffsize[i/4],elements[i/2]);
            start = times(&tmstart);
            sort_sys(arg);
            end = times(&tmstop);

        }
        else   // lib testing
        {
            fprintf(wynik,"lib, buffer - %s, elements - %s\n",buffsize[i/4],elements[i/2]);
            start = times(&tmstart);
            sort_lib(arg);
            end = times(&tmstop);
        }

        fprintf(wynik,"\n");
        fprintf(wynik,"real time: %7.2f s\n", (double)(end-start)/clktck);
        fprintf(wynik,"user time: %7.2f s\n", (double)(tmstop.tms_utime - tmstart.tms_utime)/clktck);
        fprintf(wynik,"system time: %7.2f s\n", (double)(tmstop.tms_stime - tmstart.tms_stime)/clktck);
        fprintf(wynik,"-----------------\n\n\n");

    }

    elements[0]="2048000\0";
    elements[1]="4096000\0";

    elements[2]="2048000\0";
    elements[3]="4096000\0";

    elements[4]="2048000\0";
    elements[5]="4096000\0";       // copying is faster, so i increased amounts of elements

    elements[6]="204800\0";
    elements[7]="409600\0";

    elements[8]="204800\0";
    elements[9]="409600\0";

    elements[10]="204800\0";
    elements[11]="409600\0";


    fprintf(wynik,"#####copying test######\n\n");
    for (int i=0; i<24; i++)  // copy testing
    {
        if(i%2==0)
        {                   // generating file
            arg[2]=tmp;
            arg[4]=buffsize[i/4];
            arg[3]=elements[i/2];
            generate(arg);
        }

        arg[2]=tmp;
        arg[3]=plik;
        arg[4]=elements[i/2];
        arg[5]=buffsize[i/4];

        if (i%2==0) {   // sys testing

            fprintf(wynik,"sys, buffer - %s, elements - %s",buffsize[i/4],elements[i/2]);

            start = times(&tmstart);
            copy_sys(arg);
            end = times(&tmstop);

        }
        else   // lib testing
        {
            fprintf(wynik,"lib, buffer - %s, elements - %s",buffsize[i/4],elements[i/2]);

            start = times(&tmstart);
            copy_lib(arg);
            end = times(&tmstop);
        }

        fprintf(wynik,"\n");
        fprintf(wynik,"real time: %7.2f s\n", (double)(end-start)/ clktck);
        fprintf(wynik,"user time: %7.2f s\n", (double)(tmstop.tms_utime - tmstart.tms_utime)/ clktck);
        fprintf(wynik,"system time: %7.2f s\n", (double)(tmstop.tms_stime - tmstart.tms_stime)/ clktck);
        fprintf(wynik,"-----------------\n\n\n");

    }

    if (arg!=NULL) free(arg);
    if (buffsize!=NULL) free(buffsize);
    if (elements!= NULL) free(elements);

    fclose(wynik);

}