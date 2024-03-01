#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define BUFFOR_SIZE 80

void on_usr1(int signal) {
}



int main(int argc, char **argv) {

    if(argc < 2) {
        printf("No args provided.\n");
        exit(1);
    }

    int numOfThreads = atoi(argv[1]);
    int threadIps[numOfThreads];

    if(numOfThreads == 0) {
        printf("Invalid thread number.\n");
    }

    // Wczytanie wektora z pliku

    FILE* f = fopen("vector.txt", "r");
    char buffor[BUFFOR_SIZE+1];
	double* vector;
	int n;
	fgets(buffor, BUFFOR_SIZE, f);
 	n = atoi(buffor);
	vector = malloc(sizeof(double) * n);
	printf("Vector has %d elements\n", n);
	for(int i=0; i<n; i++) {
		fgets(buffor, BUFFOR_SIZE, f);
		vector[i] = atof(buffor);
	}
	fclose(f);

    // Ustawienie 

    for(int i = 0; i < numOfThreads; i++) {

        pid_t pid = fork();

        switch(pid) {

            case -1:
                printf("Error while fork()\n");
                exit(1);
            case 0:
                int id = i * 2;
                sigset_t mask;
                struct sigaction usr1;
                sigemptyset(&mask); /* Wyczyść maskę */
                usr1.sa_handler = (&on_usr1);
                usr1.sa_mask = mask;
                usr1.sa_flags = SA_SIGINFO;
                sigaction(SIGUSR1, &usr1, NULL);
                pause();

                key_t key = 1032;
                int size = sizeof(double) * n + 1;
                int shmid;
                double *shm_vector, *shm_intervals, *shm_results, *s;


                key = 1002;
                size = sizeof(int) * (numOfThreads * 2 + 2);

                if ((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) {
                    perror("shmget");
                    exit(1);
                }
                if ((shm_intervals = shmat(shmid, NULL, 0)) == (double *) -1) {
                    perror("shmat");
                    exit(1);
                }
                s = shm_intervals;

                int begin = s[id];
                int end = s[++id];

                key = 1001;
                size = sizeof(double) * n + 1;

                if ((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) {
                    perror("shmget");
                    exit(1);
                }
                if ((shm_vector = shmat(shmid, NULL, 0)) == (double *) -1) {
                    perror("shmat");
                    exit(1);
                }
                s = shm_vector;

                double sum = 0;
                for(int i = begin; i < end; i++) {
                    sum += s[i];
                }

                // Utworzenie przestrzeni pamięci współdzielonej dla wyników

                key = 1003;
                size = sizeof(double) * numOfThreads + 1;

                if ((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0) {
                    perror("shmget");
                    exit(1);
                }
                if ((shm_results = shmat(shmid, NULL, 0)) == (double *) -1) {
                    perror("shmat");
                    exit(1);
                }
                s = shm_results;
                s[i] = sum;
                
                if (shmdt(shm_vector) == -1) {
                    perror("shmdt vector");
                    exit(1);
                }

                if (shmdt(shm_intervals) == -1) {
                    perror("shmdt intervals");
                    exit(1);
                }

                if (shmdt(shm_results) == -1) {
                    perror("shmdt results");
                    exit(1);
                }
                return 0;

            default:
                threadIps[i] = pid;
        }
    }

    // Utworzenie przestrzeni pamięci współdzielonej dla wektora

    key_t key = 1001;
    int size = sizeof(double) * n + 1;
    int shmid_vector, shmid_intervals, shmid_results;
    double *shm_vector, *shm_intervals, *shm_results, *s;

    if ((shmid_vector = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shm_vector = shmat(shmid_vector, NULL, 0)) == (double *) -1) {
        perror("shmat");
        exit(1);
    }
    s = shm_vector;

    for(int i = 0; i < n; i ++) {
        *s = vector[i];
        s++;
    }
    

    // Utworzenie przestrzeni pamięci współdzielonej dla przedziałów

    key = 1002;
    size = sizeof(int) * (numOfThreads * 2 + 2);

    if ((shmid_intervals = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shm_intervals = shmat(shmid_intervals, NULL, 0)) == (double *) -1) {
        perror("shmat");
        exit(1);
    }
    s = shm_intervals;

    // divide the vector into parts
    int rest = n % numOfThreads;
    int part = (n - rest) / numOfThreads;
    int addition = 0;

    for(int i = 0; i < numOfThreads; i++) {
        if(rest > 0) {
            *s++ = i * part + addition;    
            *s++ = i * part + part + addition + 1;
            addition++;
            rest--;
        } else {
            *s++ = i * part + addition;
            *s++ = i * part + part + addition;
        }
    }
    s = shm_intervals;

    while(rest > 0) {
        *s++ = *s + 1;
        rest--;
    }

    // Handle the remaining elements
    if (rest > 0) {
        *s++ = numOfThreads * part;
        *s++ = numOfThreads * part + rest;
    }


    // Utworzenie przestrzeni pamięci współdzielonej dla wyników

    key = 1003;
    size = sizeof(double) * (numOfThreads + 1);

    if ((shmid_results = shmget(key, size, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shm_results = shmat(shmid_results, NULL, 0)) == (double *) -1) {
        perror("shmat");
        exit(1);
    }


    // Wysłanie sygnału

    struct timeval start, end;
    double elapsed;
    gettimeofday(&start, NULL);

    for(int i = 0; i < numOfThreads; i++) {
        kill(threadIps[i], SIGUSR1);
    }

    for(int i = 0; i < numOfThreads; i++) {
        wait(0);
    }

    s = shm_results;
    int sum = 0;
    for(int i = 0; i < numOfThreads; i++) {
        int num = *s++;
        sum += num;
    }
    gettimeofday(&end, NULL);
    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("\nSUM: %d Time: %f\n", sum, elapsed);

    shmctl(shmid_vector, IPC_RMID, NULL);
    shmctl(shmid_intervals, IPC_RMID, NULL);
    shmctl(shmid_results, IPC_RMID, NULL);

    return 0;
}