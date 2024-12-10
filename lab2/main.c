#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>


typedef struct {
    long long *array;
    long long left;
    long long right;
} ThreadData;


sem_t thread_limit;

void merge(long long *array, long long left, long long mid, long long right) {
    long long i = left, j = mid + 1, k = 0;
    long long *temp = (long long *)malloc((right - left + 1) * sizeof(long long));

    while (i <= mid && j <= right) {
        if (array[i] <= array[j]) {
            temp[k++] = array[i++];
        } else {
            temp[k++] = array[j++];
        }
    }

    while (i <= mid) temp[k++] = array[i++];
    while (j <= right) temp[k++] = array[j++];

    for (i = left, k = 0; i <= right; i++, k++) {
        array[i] = temp[k];
    }

    free(temp);
}

void tim_sort(long long *array, long long left, long long right) {
    if (left >= right) return;

    long long mid = left + (right - left) / 2;
    tim_sort(array, left, mid);
    tim_sort(array, mid + 1, right);
    merge(array, left, mid, right);
}

void *thread_sort(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    tim_sort(data->array, data->left, data->right);
    sem_post(&thread_limit);
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        const char msg[] = "Usage: <max_threads> <array_size>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    long long array_size = atoll(argv[2]);
    int max_threads = atoi(argv[1]);

    if (max_threads <= 0) {
        const char msg[] = "Incorrect number of threads\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return EXIT_FAILURE;
    }

    pthread_t threads[max_threads];


    sem_init(&thread_limit, 0, max_threads);

    long long *array = (long long *)malloc(array_size * sizeof(long long));
    for (long long i = 0; i < array_size; i++) {
        array[i] = rand() % 1000000;
    }

    long long chunk_size = array_size / max_threads;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < max_threads; i++) {
        long long left = i * chunk_size;
        long long right = (i == max_threads - 1) ? array_size - 1 : (left + chunk_size - 1);

        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        data->array = array;
        data->left = left;
        data->right = right;

        sem_wait(&thread_limit);
        pthread_create(&threads[i], NULL, thread_sort, data);
    }

    for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);

    double time_taken = ((end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec)) / 1000000.0;
    printf("Sorting completed in %.6f seconds.\n", time_taken);

    free(array);
    sem_destroy(&thread_limit);

    return EXIT_SUCCESS;
}
