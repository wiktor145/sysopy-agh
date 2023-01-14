//
// Created by wicia on 18.05.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int main(void)
{
    srand(time(NULL));

    for (int i=3; i<=65; i++)
    {
        double *T = malloc(i*i*sizeof(double));

        int sum = 0;

        for (int a=0; a<i*i; a++)
        {
            T[a]= rand()%100;
            sum+=T[a]*T[a];
        }

        double length = sqrt(sum);

        for (int a=0; a<i*i; a++)
        {
            T[a] /= length;
            T[a] *= T[a];
        }
        char nazwa[25];
        sprintf(nazwa,"./filtry/filtr%d.txt",i);

        FILE *f = fopen(nazwa,"w");

        fprintf(f,"%d\n",i);

        for (int a=0; a<i*i; a++)
            fprintf(f,"%lf\n",T[a]);

        fclose(f);


        free(T);
    }

    return 0;
}
