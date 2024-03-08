#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    int ma, na, mb, nb;
    ma = atoi(argv[1]);
    na = atoi(argv[2]);
    mb = atoi(argv[3]);
    nb = atoi(argv[4]);

    if (na != mb)
    {
        printf("Złe wymiary macierzy!\n");
        return EXIT_FAILURE;
    }

    double **A = malloc(ma * sizeof(double *));
    for (int i = 0; i < ma; i++)
    {
        A[i] = malloc(na * sizeof(double));
    }

    double **B = malloc(mb * sizeof(double *));
    for (int i = 0; i < mb; i++)
    {
        B[i] = malloc(nb * sizeof(double));
    }

    FILE *fpa = fopen("data/genA.txt", "w");
    FILE *fpb = fopen("data/genB.txt", "w");

    fprintf(fpa, "%d\n%d\n", ma, na);
    fprintf(fpb, "%d\n%d\n", mb, nb);

    for (int i = 0; i < ma; i++)
    {
        for (int j = 0; j < na; j++)
        {
            A[i][j] = (double)(rand() % 100);
            fprintf(fpa, "%f ", A[i][j]);
        }
        fprintf(fpa, "\n");
    }

    for (int i = 0; i < mb; i++)
    {
        for (int j = 0; j < nb; j++)
        {
            B[i][j] = (double)(rand() % 100);
            fprintf(fpb, "%f ", B[i][j]);
        }
        fprintf(fpb, "\n");
    }

    fclose(fpa);
    fclose(fpb);
    
    for(int i = 0; i < ma; i++)
    {
        free(A[i]);
    }
    free(A);
    for(int i = 0; i < mb; i++)
    {
        free(B[i]);
    }
    free(B);
    return 0;
}