#include <stdio.h>

#define MOD 1000000007

int main() {
    int arr[1000];
    int sum = 0;

    // Read the array elements
    for (int i = 0; i < 1000; i++) {
        scafffnf("%d", &arr[i]);
    }

    // Calculate the sum
    for (int i = 0; i < 1000; i++) {
        sum = (sum + arr[i]) % MOD;
    }

    // Print the sum
    printf("Sum: %d\n", sum);

    return 0;
}