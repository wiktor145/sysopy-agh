
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>

enum option{
    BLOCK = 1,
    INTERLEAVED = 2
};

enum option OPTION;

int threads_no;

short ** IMAGE;

int W,H,C;

//image is WxH
//filter is CxC

short ** IMAGE_FILTERED;

pthread_t * threads;

int * threads_id;

double** FILTER;

int max(int a, int b)
{
    return a > b ? a : b;
}

int min (int a,int b)
{
    return a < b ? a : b;
}

int my_ceil(int a,int b)
{
    int c = a/b;
    return c*b == a ? c : c+1;
}


double value(int x, int y)
{
    double ret = 0.0;
    int c2 = (int) my_ceil(C,2);

    for (int i=0; i<C; i++)
        for (int j=0; j<C; j++)
        {
            ret+=(double)IMAGE[min(H-1,max(0,x-1-c2+i))][min(W-1,max(0,y-1-c2+j))]*FILTER[i][j];
        }
    return ret;

}


void * thread_start(void * args)
{
    struct timeval start;
    gettimeofday(&start,NULL);

    int my_nr = *(int*)args;

    if (OPTION == BLOCK)
    {
        int left = (my_nr-1)*(int)(my_ceil(W,threads_no));
        int right = my_nr*(int)(my_ceil(W,threads_no))-1;
        //printf("%d %d\n",left,right);
        for (int x = 0; x<H; ++x)
            for (int y = left; y<=right && y < W; ++y)
                IMAGE_FILTERED[x][y] = max(0,min(255,(int)round(value(x,y))));

    }
    else
    {

        for (int x = 0; x<H; ++x)
            for (int y = my_nr-1; y<W; y+=threads_no)
                IMAGE_FILTERED[x][y] = max(0,min(255,(int)round(value(x,y))));


    }


    struct timeval end;
    gettimeofday(&end,NULL);

    struct timeval *ret = malloc(sizeof(struct timeval));
    timersub(&end,&start,ret);

    pthread_exit((void*)ret);
}

int handle_input_file(FILE* f)
{
    int P;
    fscanf(f, "P%d\n", &P);
    if (P!= 2)
    {
        printf("Error in file!\n");
        return -1;
    }

    int m;
    fscanf(f, "%d", &W);
    fscanf(f, "%d", &H);
    fscanf(f, "%d", &m);

   // printf("%d %d\n",W,H);

    if (m!= 255)
    {
        printf("Error in file!\n");
        return -1;
    }

    IMAGE = malloc(H*sizeof(short*));
    IMAGE_FILTERED = malloc(H*sizeof(short*));
    for (int i=0; i<H; i++)
    {
        IMAGE[i] = (short *) malloc(W * sizeof(short));
        IMAGE_FILTERED[i] = (short *) malloc(W * sizeof(short));
    }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            fscanf(f, "%hd", &IMAGE[y][x]);


    return 0;
}


int handle_filter(FILE* f)
{
    fscanf(f, "%d", &C);
    FILTER = malloc(sizeof(double)*C);
    for (int i=0; i<C; i++)
        FILTER[i] = malloc(C*sizeof(double));

    for (int y = 0; y < C; ++y)
        for (int x = 0; x < C; ++x)
                fscanf(f, "%lf", &FILTER[y][x]);

    return 0;
}

void handle_number(int number)
{
    threads = malloc(number*sizeof(pthread_t));
    threads_id = malloc(number*sizeof(int));

    for (int i=0; i<number; i++)
    {
        threads_id[i]=i+1;
    }

}

void save_image(FILE* f)
{
    fprintf(f, "P2\n%d %d\n%d\n", W, H, 255);

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            fprintf(f, "%d ", IMAGE_FILTERED[y][x]);
        }
        fprintf(f, "\n");

    }

}


void time_get(struct timeval start, struct timeval end)
{
    struct timeval curTime;

    timersub(&end,&start,&curTime);

    printf("Whole time:\n");
    printf("%ld s %ld us\n",curTime.tv_sec,curTime.tv_usec);
}


int main(int argc, char** argv)
{
    if (argc != 6 && argc!= 7)
    {
        printf("Error! Program should be executed with 5 (6) arguments:\n");
        printf("1 - number of threads\n");
        printf("2 - method of image split: block or interleaved\n");
        printf("3 - name of input image file\n");
        printf("4 - name of filter file\n");
        printf("5 - name of output file\n");
        printf("If you give 6-th argument, time will be saved to Times.txt\n");
        exit(1);
    }

    threads_no = atoi(argv[1]);

    if (threads_no == 0)
    {
        printf("Error in number of threads %s\n",argv[1]);
        exit(1);
    }

    if (strcmp("block\0",argv[2]) != 0 && strcmp("interleaved\0",argv[2]) != 0)
    {
        printf("Error! Second argument should be block or interleaved\n");
        exit(1);
    }

    if (strcmp("block\0",argv[2]) == 0)
    {
        OPTION = BLOCK;
    }
    else
        OPTION = INTERLEAVED;


    FILE *input = fopen(argv[3],"r");
    if (!input)
    {
        printf("Error during opening input file %s\n",argv[3]);
        exit(1);
    }

    FILE *output = fopen(argv[5],"w+");
    if (!output)
    {
        printf("Error during creating output file %s\n",argv[5]);
        fclose(input);
        exit(1);
    }

    FILE *filter = fopen(argv[4],"r");
    if (!filter)
    {
        printf("Error during opening filter file %s\n",argv[4]);
        fclose(input);
        fclose(output);
        exit(1);
    }

    if (handle_input_file(input) == -1)
    {
        fclose(input);
        fclose(output);
        fclose(filter);
        exit(1);
    }

    if (handle_filter(filter) == -1)
    {
        fclose(input);
        fclose(output);
        fclose(filter);
        exit(1);
    }

    handle_number(threads_no);


    struct timeval start;
    gettimeofday(&start,NULL);

    for (int i=0; i<threads_no; i++)
        if (pthread_create(&threads[i],NULL,thread_start,(void*)&threads_id[i]) != 0)
        {
            printf("Error during creating threads\n");
            fclose(input);
            fclose(output);
            fclose(filter);
            exit(1);
        }

    struct timeval *thread_time;
    void* res;

    for (int i=0; i<threads_no; i++) {

        pthread_join(threads[i], &res);
        thread_time = (struct timeval *) res;
        if (argc == 6) {
            printf("Thread %lu finished with time:\n", threads[i]);
            printf("%ld s %ld us\n", thread_time->tv_sec, thread_time->tv_usec);
        }
        free(res);
    }
    struct timeval end;
    gettimeofday(&end,NULL);
    if (argc == 6)
        time_get(start,end); //albo na opak...
    else
    {
        struct timeval curTime;

        timersub(&end,&start,&curTime);
        FILE * f = fopen("Times.txt","a");
        fprintf(f,"Type: %s Threads: %d Filter size: %d Time: %ld s %ld us\n\n",
                OPTION == BLOCK ? "block\0": "Interleaved\0",
                threads_no,C,curTime.tv_sec,curTime.tv_usec);
        fclose(f);
    }

    save_image(output);

    //free(res);
    //free(thread_time);
    for (int a = 0; a< H; a++)
    {
        free(IMAGE_FILTERED[a]);
        free(IMAGE[a]);
    }
    free(IMAGE_FILTERED);
    free(IMAGE);
    for (int a=0; a< C; a++)
        free(FILTER[a]);

    free(FILTER);
    free(threads_id);
    free(threads);
    fclose(input);
    fclose(output);
    fclose(filter);
    return 0;
}