#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils.h"

#define RUN 512

sem_t semaphore;

typedef struct {
    long long* arr;
    long long left;
    long long right;
} ThreadArgs;


void insertionSort(long long arr[], long long left, long long right) {
    for (long long i = left + 1; i <= right; i++) {
        long long temp = arr[i];
        long long j = i - 1;
        while (j >= left && arr[j] > temp) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = temp;
    }
}


void* insertionSortThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*) arg;
    insertionSort(args->arr, args->left, args->right);
    sem_post(&semaphore);
    return NULL;
}


void merge(long long arr[], long long l, long long m, long long r) {
    long long len1 = m - l + 1, len2 = r - m;
    long long* left = (long long*) malloc(len1 * sizeof(long long));
    long long* right = (long long*) malloc(len2 * sizeof(long long));
    memcpy(left, arr + l, len1 * sizeof(long long));
    memcpy(right, arr + m + 1, len2 * sizeof(long long));

    long long i = 0, j = 0, k = l;

    while (i < len1 && j < len2) {
        if (left[i] <= right[j]) {
            arr[k++] = left[i++];
        } else {
            arr[k++] = right[j++];
        }
    }

    while (i < len1) {
        arr[k++] = left[i++];
    }

    while (j < len2) {
        arr[k++] = right[j++];
    }

    free(left);
    free(right);
}


void timSort(long long arr[], long long n, int numThreads) {
    long long numSegments = (n + RUN - 1) / RUN;

    pthread_t* threads = (pthread_t*) malloc(numSegments * sizeof(pthread_t));
    ThreadArgs* threadArgs = (ThreadArgs*) malloc(numSegments * sizeof(ThreadArgs));
    
    if (threads == NULL || threadArgs == NULL) {
        const char error[] = "Memory allocation failed for threads or threadArgs\n";
        write(STDERR_FILENO, error, sizeof(error));
        free(threads);
        free(threadArgs);
        return;
    }

    long long threadIndex = 0;

    for (long long i = 0; i < n; i += RUN) {
        long long left = i;
        long long right = (i + RUN - 1) < (n - 1) ? (i + RUN - 1) : (n - 1);
        
        sem_wait(&semaphore);

        threadArgs[threadIndex].arr = arr;
        threadArgs[threadIndex].left = left;
        threadArgs[threadIndex].right = right;

        int ret = pthread_create(&threads[threadIndex], NULL, insertionSortThread, (void*) &threadArgs[threadIndex]);

        threadIndex++;
    }

    for (long long i = 0; i < numSegments; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(threadArgs);

    for (long long size = RUN; size < n; size *= 2) {
        for (long long left = 0; left < n; left += 2 * size) {
            long long mid = left + size - 1;
            long long right = (left + 2 * size - 1) < (n - 1) ? (left + 2 * size - 1) : (n - 1);

            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        const char msg[] = "Usage: %s <array_size> <number_of_threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return 1;
    }

    long long size = atoll(argv[1]);
    int numThreads = atoi(argv[2]);

    long long* arr = (long long*) malloc(size * sizeof(long long));
    if (arr == NULL) {
        const char error[] = "Memory allocation failed\n";
        write(STDERR_FILENO, error, sizeof(error));
        return 1;
    }

    srand(time(NULL));

    for (long long i = 0; i < size; i++) {
        arr[i] = (long long) rand() * RAND_MAX + rand();
    }

    sem_init(&semaphore, 0, numThreads);

    struct timespec start_time, end_time;
    double elapsed;

    clock_gettime(CLOCK_REALTIME, &start_time);

    timSort(arr, size, numThreads);

    clock_gettime(CLOCK_REALTIME, &end_time);

    if (end_time.tv_nsec < start_time.tv_nsec) {
        end_time.tv_sec -= 1;
        end_time.tv_nsec += 1000000000;
    }
    elapsed = (end_time.tv_sec - start_time.tv_sec) +
              (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    print_time(elapsed);

    free(arr);
    sem_destroy(&semaphore);


    return 0;
}

