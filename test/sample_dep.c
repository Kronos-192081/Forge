#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <length>\n", argv[0]);
        return 1;
    }

    int length = atoi(argv[1]);
    if (length <= 0) {
        printf("Invalid length\n");
        return 1;
    }

    int *array = (int *)malloc(length * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    srand(time(NULL));
    for (int i = 0; i < length; i++) {
        array[i] = rand();
    }

    printf("Random array of length %d:\n", length);
    for (int i = 0; i < length; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    free(array);

    return 0;
}