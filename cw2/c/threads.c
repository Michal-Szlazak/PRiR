#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct matrix {
    int m;
    int n;
    double** values;
};

struct thread_data {
    int* intervals;
    int threadIndex;
    struct matrix A;
    struct matrix B;
    struct matrix C;
};

struct forbenius_data {
    int* intervals;
    int threadIndex;
    struct matrix C;
    double *result;
};

void print_matrix(struct matrix A);
int* create_intervals(int maxIndex, int numOfThreads);
void* write_to_result(void *threadarg);
void* calculate_forbenius(void *threadarg);

int main (int argc, char *argv[]) {
    
    FILE *fpa;
    FILE *fpb;

    struct matrix A;
    struct matrix B;
    struct matrix C;

    if(argc != 3) {
        printf("Not enough arguments\n Arguments should be: <number of threads> <mode>, where mode -> [base, gen]\n");
        return EXIT_FAILURE;
    }

    int i, j;
    double x;

    if(strcmp(argv[2], "base") == 0) {
        fpa = fopen("data/A.txt", "r");
        fpb = fopen("data/B.txt", "r");
    } else if(strcmp(argv[2], "gen") == 0) {
        fpa = fopen("data/genA.txt", "r");
        fpb = fopen("data/genB.txt", "r");
    } else {
        printf("Wrong mode\n");
        return EXIT_FAILURE;
    }

    if( fpa == NULL || fpb == NULL )
    {
        perror("błąd otwarcia pliku");
        exit(-10);
    }

    fscanf (fpa, "%d", &A.m);
    fscanf (fpa, "%d", &A.n);

    fscanf (fpb, "%d", &B.m);
    fscanf (fpb, "%d", &B.n);

    if(A.n != B.m)
    {
        printf("Złe wymiary macierzy!\n");
        return EXIT_FAILURE;
    }

      /*Alokacja pamięci*/
    A.values = malloc(A.m*sizeof(double));
    for(i=0; i< A.m; i++)
    {
        A.values[i] = malloc(A.n*sizeof(double));
    }

    B.values = malloc(B.m*sizeof(double));
    for(i=0; i< B.m; i++)
    {
        B.values[i] = malloc(B.n*sizeof(double));
    }

    /*Macierz na wynik*/
    C.m = A.m;
    C.n = B.n;
    C.values = malloc(C.m*sizeof(double));
    for(i=0; i< A.m; i++)
    {
        C.values[i] = malloc(C.n*sizeof(double));
    }

    for(i =0; i< A.m; i++)
    {
        for(j = 0; j<A.n; j++)
        {
            fscanf( fpa, "%lf", &x );
            A.values[i][j] = x;
        }
    }
    
    for(i =0; i< B.m; i++)
    {
        for(j = 0; j<B.n; j++)
        {
            fscanf( fpb, "%lf", &x );
            B.values[i][j] = x;
        }
    }

    int maxIndex = C.m * C.n;
    int numOfThreads = atoi(argv[1]);

    int *intervals = create_intervals(maxIndex, numOfThreads);

    pthread_t threads[numOfThreads];

    struct timeval begin, end;
    gettimeofday(&begin, 0);

    for(int i = 0; i < numOfThreads; i++) {

        struct thread_data *data = malloc(sizeof(struct thread_data));
        data->intervals = intervals;
        data->threadIndex = i;
        data->A = A;
        data->B = B;
        data->C = C;

        pthread_create(&threads[i], NULL, write_to_result, (void *)data);
    }

    for(int i = 0; i < numOfThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    double *result = malloc(sizeof(double));
    *result = 0;

    for(int i = 0; i < numOfThreads; i++) {

        struct forbenius_data *data = malloc(sizeof(struct forbenius_data));
        data->intervals = intervals;
        data->threadIndex = i;
        data->C = C;
        data->result = result;

        pthread_create(&threads[i], NULL, calculate_forbenius, (void *)data);
    }

    for(int i = 0; i < numOfThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    *result = sqrt(*result);

    gettimeofday(&end, 0);
    double elapsed = (end.tv_sec - begin.tv_sec) +
              (end.tv_usec - begin.tv_usec) / 1000000.0;

    printf("Time: %f result: %f\n", elapsed, *result);
    // print_matrix(C);
}

void print_matrix(struct matrix A)
{
    int i, j;
    printf("[");
    for(i =0; i< A.m; i++)
    {
        for(j=0; j<A.n; j++)
        {
            printf("%f ", A.values[i][j]);
        }
        printf("\n");
    }
    printf("]\n");
}

int* create_intervals(int maxIndex, int numOfThreads){

    int *s = malloc(sizeof(int) * (numOfThreads * 2));

    int rest = maxIndex % numOfThreads;
    int part = (maxIndex - rest) / numOfThreads;
    int addition = 0;
    int index = 0;

    for(int i = 0; i < numOfThreads; i++) {
        if(rest > 0) {
            s[index] = i * part + addition;    
            s[++index] = i * part + part + addition + 1;
            addition++;
            rest--;
        } else {
            s[index] = i * part + addition;
            s[++index] = i * part + part + addition;
        }

        index++;
    }

    return s;
}

void* write_to_result(void *threadarg) {

    struct thread_data *data;
    data = (struct thread_data *) threadarg;

    int *intervals = data->intervals;
    int threadIndex = data->threadIndex;
    struct matrix C = data->C;
    struct matrix A = data->A;
    struct matrix B = data->B;

    int start = intervals[threadIndex * 2];
    int end = intervals[threadIndex * 2 + 1];

    for(int i = start; i < end; i++) {
        int row = i / C.n;
        int col = i % C.n;

        double s = 0;
        for(int k = 0; k < A.n; k++) {
            s += A.values[row][k] * B.values[k][col];
        }
        
        pthread_mutex_lock(&mutex);
        C.values[row][col] = s;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void* calculate_forbenius(void *threadarg) {

    struct forbenius_data *data;
    data = (struct forbenius_data *) threadarg;

    int *intervals = data->intervals;
    int threadIndex = data->threadIndex;
    struct matrix C = data->C;
    double result = 0;

    int start = intervals[threadIndex * 2];
    int end = intervals[threadIndex * 2 + 1];

    double s = 0;
    for(int i = start; i < end; i++) {
        int row = i / C.n;
        int col = i % C.n;
        s += fabs(C.values[row][col] * C.values[row][col]);
        // printf("[%d][%d] = %f^2 = %f\n", row, col, C.values[row][col], fabs(C.values[row][col] * C.values[row][col]));
    }

    // printf("Thread %d: %f\n", threadIndex, s);

    pthread_mutex_lock(&mutex);
    *(data->result) += s;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}