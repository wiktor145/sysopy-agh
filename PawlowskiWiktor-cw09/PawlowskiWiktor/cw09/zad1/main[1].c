#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

int all_started = 0;


int trolleys_number;
pthread_mutex_t trolleys_number_mutex = PTHREAD_MUTEX_INITIALIZER;

int trolleys_number_constant;

int passengers_number, passengers_number_constant;
int trolley_capacity;
int number_of_rides;

int passengers_inside = 0;
pthread_mutex_t passengers_inside_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t passengers_inside_cond   = PTHREAD_COND_INITIALIZER;

int places_left_inside = 0;
pthread_mutex_t places_left_inside_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t places_left_inside_cond   = PTHREAD_COND_INITIALIZER;


pthread_t * trolleys;
pthread_t * passengers;

// 0 do not get out from the trolley
// 1 do get out
short* get_out;
pthread_mutex_t* get_out_mutex;
pthread_cond_t* get_out_cond;

int current_trolley = 0;
pthread_mutex_t current_trolley_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t current_trolley_cond   = PTHREAD_COND_INITIALIZER;


int current_passenger = 0;
pthread_mutex_t current_passenger_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t current_passenger_cond   = PTHREAD_COND_INITIALIZER;

int* ids_for_threads;

void time_get(void)
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);

    char buffer [80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

    printf("%s:%ld\n\n", buffer, curTime.tv_usec/1000);

}



static void * trolley(void * args)
{
    int my_id = *((int*) args);

    while (all_started == 0) usleep(100);

    for (int i=0; i<number_of_rides + 1; i++)
    {
        //dostan swoj id
        pthread_mutex_lock(&current_trolley_mutex);
        while (current_trolley != my_id) {
            pthread_cond_wait(&current_trolley_cond, &current_trolley_mutex);
        }


        if (i > 0)
        {
            //wypusc
            printf("Koniec jazdy - wagonik %d, czas: ",my_id);
            time_get();

            pthread_mutex_lock(&passengers_inside_mutex);

            passengers_inside = trolley_capacity;

            pthread_mutex_unlock(&passengers_inside_mutex);

            pthread_mutex_lock(&get_out_mutex[my_id]);
            get_out[my_id] = 1;

            printf("Otwarcie drzwi - wagonik %d, czas: ",my_id);
            time_get();


            pthread_cond_broadcast(&get_out_cond[my_id]);

            pthread_mutex_unlock(&get_out_mutex[my_id]);

            pthread_mutex_lock(&passengers_inside_mutex);
            while (passengers_inside > 0) {
                pthread_cond_wait(&passengers_inside_cond, &passengers_inside_mutex);
            }

            pthread_mutex_lock(&get_out_mutex[my_id]);
            get_out[my_id] = 0;

            pthread_mutex_unlock(&get_out_mutex[my_id]);

            pthread_mutex_unlock(&passengers_inside_mutex);

        }

        if (i < number_of_rides)
        {
            //wpusc i pojedz jeszcze raz
            pthread_mutex_lock(&places_left_inside_mutex);

            printf("Wagonik %d zaprasza na poklad, czas: ",my_id);
            time_get();


            places_left_inside=trolley_capacity;

            pthread_cond_broadcast(&places_left_inside_cond);

            pthread_mutex_unlock(&places_left_inside_mutex);

            pthread_mutex_lock(&passengers_inside_mutex);
            while (passengers_inside < trolley_capacity)
            {
                pthread_cond_wait(&passengers_inside_cond, &passengers_inside_mutex);
            }

            printf("Wagonik %d zamyka drzwi, czas: ",my_id);
            time_get();


            passengers_inside = 0;

            pthread_mutex_unlock(&passengers_inside_mutex);

            printf("Wagonik %d rozpoczyna jazde, czas: ",my_id);
            time_get();

        }
        else
        {
            //zakoncz dzialanie
            pthread_mutex_lock(&trolleys_number_mutex);

            trolleys_number--;

            printf("Zakonczenie pracy - wagonik %d, czas: ",my_id);
            time_get();


            if (trolleys_number == 0)
            {

                pthread_mutex_lock(&places_left_inside_mutex);

                places_left_inside=trolley_capacity;

                pthread_cond_broadcast(&places_left_inside_cond);

                pthread_mutex_unlock(&places_left_inside_mutex);

            }

            pthread_mutex_unlock(&trolleys_number_mutex);

        }

        current_trolley = (current_trolley + 1 ) % trolleys_number_constant;

        pthread_cond_broadcast(&current_trolley_cond);

        pthread_mutex_unlock(&current_trolley_mutex);
    }

    pthread_exit(0);

}

static void * passenger(void * args)
{
    int my_id = *((int*) args);

    int my_trolley;


    if (my_id == passengers_number -1)
        all_started = 1;

    while (all_started == 0) usleep(100);

    while (1)
    {

        pthread_mutex_lock(&current_passenger_mutex);
        while (current_passenger != my_id) {
            pthread_cond_wait(&current_passenger_cond, &current_passenger_mutex);
        }

        pthread_mutex_lock(&trolleys_number_mutex);

        if (trolleys_number == 0)
        {
            current_passenger = (current_passenger + 1) % passengers_number_constant;

            printf("Zakonczenie pracy - pasazer %d, czas: ",my_id);
            time_get();


            pthread_mutex_unlock(&trolleys_number_mutex);

            pthread_cond_broadcast(&current_passenger_cond);

            pthread_mutex_unlock(&current_passenger_mutex);
            break;
        }
        else
            pthread_mutex_unlock(&trolleys_number_mutex);



        pthread_mutex_lock(&places_left_inside_mutex);
        while (places_left_inside == 0) {
            pthread_cond_wait(&places_left_inside_cond, &places_left_inside_mutex);
        }

        pthread_mutex_lock(&trolleys_number_mutex);

        if (trolleys_number == 0)
        {
            current_passenger = (current_passenger + 1) % passengers_number_constant;
            printf("Zakonczenie pracy - pasazer %d, czas: ",my_id);
            time_get();


            pthread_mutex_unlock(&trolleys_number_mutex);
            pthread_mutex_unlock(&places_left_inside_mutex);

            pthread_cond_broadcast(&current_passenger_cond);
            pthread_mutex_unlock(&current_passenger_mutex);
            break;
        }
        else
            pthread_mutex_unlock(&trolleys_number_mutex);


        places_left_inside--;
        my_trolley = current_trolley;
        printf("Wejscie pasazera %d do wagonika %d\n",my_id,my_trolley);
        printf("Pozostalo miejsc: %d\n",places_left_inside);
        printf("Czas: ");
        time_get();

        pthread_mutex_lock(&passengers_inside_mutex);

        passengers_inside++;

        if (passengers_inside == trolley_capacity)
        {
            printf("Nacisniecie przycisku START\n");
            printf("Pasazer %d, czas: ",my_id);
            time_get();

            pthread_cond_broadcast(&passengers_inside_cond);
        }

        pthread_mutex_unlock(&passengers_inside_mutex);

        pthread_mutex_unlock(&places_left_inside_mutex);

        current_passenger = (current_passenger + 1) % passengers_number_constant;

        pthread_cond_broadcast(&current_passenger_cond);

        pthread_mutex_unlock(&current_passenger_mutex);

        pthread_mutex_lock(&get_out_mutex[my_trolley]);
        while (get_out[my_trolley] == 0) {
            pthread_cond_wait(&get_out_cond[my_trolley], &get_out_mutex[my_trolley]);
        }

        pthread_mutex_lock(&passengers_inside_mutex);

        passengers_inside--;

        printf("Wyjscie pasazera %d z wagonika %d\n",my_id,my_trolley);
        printf("Pozostalo wewnatrz: %d\n",passengers_inside);
        printf("Czas: ");
        time_get();


        if (passengers_inside == 0)
            pthread_cond_broadcast(&passengers_inside_cond);

        pthread_mutex_unlock(&passengers_inside_mutex);

        pthread_mutex_unlock(&get_out_mutex[my_trolley]);

    }

    pthread_exit(0);

}


int initialize(void);

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        printf("Error! Program should be executed\n");
        printf("With 4 arguments:\n");
        printf("1. - number of passengers (must be equal or greater\n");
        printf("third argument times second argument)\n");
        printf("2. - number of trolleys\n");
        printf("3. - capacity of trolley\n");
        printf("4. - number of rides\n");
        exit(1);
    }

    passengers_number = atoi(argv[1]);
    trolleys_number = atoi(argv[2]);
    trolley_capacity = atoi(argv[3]);
    number_of_rides = atoi(argv[4]);

    if (passengers_number <= 0 || trolley_capacity <= 0 ||
        trolleys_number <= 0 || number_of_rides <= 0)
    {
        printf("Some error in arguments\n");
        exit(1);
    }

    if (passengers_number < trolleys_number*trolley_capacity)
    {
        printf("Error. Number of passengers can not be smaller\n");
        printf("than number of trolleys times trolley's capacity\n");
        exit(2);
    }

    passengers_number_constant = passengers_number;
    trolleys_number_constant = trolleys_number;

    if (initialize() != 0)
    {
        printf("Error during initializing values\n");
        exit(3);
    }

    for (int i=0; i<trolleys_number; i++)
        pthread_create(&trolleys[i], NULL, trolley, (void *)&ids_for_threads[i]);

    for (int i=0; i<passengers_number; i++)
        pthread_create(&passengers[i], NULL, passenger, (void *)&ids_for_threads[i]);

    for (int i=0; i<trolleys_number; i++)
        pthread_join(trolleys[i], NULL);

    for (int i=0; i<passengers_number; i++)
        pthread_join(passengers[i], NULL);


    free(trolleys);
    free(passengers);
    free(get_out);
    free(get_out_mutex);
    free(get_out_cond);
    free(ids_for_threads);

    return 0;
}


int initialize(void)
{
    trolleys = malloc(trolleys_number * sizeof(pthread_t));

    if (trolleys == NULL)
        return 1;

    passengers = malloc(passengers_number * sizeof(pthread_t));

    if (passengers == NULL)
    {
        free(trolleys);
        return 1;
    }

    get_out = malloc(trolleys_number * sizeof(short));
    if (get_out == NULL)
    {
        free(trolleys);
        free(passengers);
        return 1;
    }

    for (int i=0; i<trolleys_number; i++ )
        get_out[i] = 0;

    get_out_mutex = malloc(trolleys_number * sizeof(pthread_mutex_t));

    if (get_out_mutex == NULL)
    {
        free(trolleys);
        free(passengers);
        free(get_out);

        return 1;
    }

    for (int i=0; i<trolleys_number; i++ )
        pthread_mutex_init ( &get_out_mutex[i], NULL);

    get_out_cond = malloc(trolleys_number * sizeof(pthread_cond_t));

    if (get_out_cond == NULL)
    {
        free(trolleys);
        free(passengers);
        free(get_out);
        free(get_out_mutex);
        return 1;
    }

    for (int i=0; i<trolleys_number; i++ )
        pthread_cond_init(&get_out_cond[i],NULL);

    ids_for_threads = malloc(passengers_number * sizeof(int));
    if (ids_for_threads == NULL)
    {
        free(trolleys);
        free(passengers);
        free(get_out);
        free(get_out_mutex);
        free(get_out_cond);

        return 1;
    }

    for (int i =0; i<passengers_number; i++)
        ids_for_threads[i] = i;

    return 0;
}