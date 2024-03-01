#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {

    FILE *file;
    int numNumbers = atoi(argv[1]);

    srand(time(NULL));

    file = fopen("vector.txt", "w");
    if (file == NULL) {
        printf("Error opening file.\n");
        return 1;
    }

    fprintf(file, "%d\n", numNumbers);

    // Generate and write random numbers to the file
    for (int i = 0; i < numNumbers; i++) {
        int randomNumber = rand() % 10;
        fprintf(file, "%d\n", randomNumber);
    }

    // Close the file
    fclose(file);
    return 0;
}