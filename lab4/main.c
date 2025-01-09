#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <time.h>

typedef struct Allocator Allocator;

typedef Allocator* (*allocator_create_t)(void* const memory, const size_t size);
typedef void (*allocator_destroy_t)(Allocator* const allocator);
typedef void* (*allocator_alloc_t)(Allocator* const allocator, const size_t size);
typedef void (*allocator_free_t)(Allocator* const allocator, void* const memory);

Allocator* fallback_allocator_create(void* const memory, const size_t size) {
    printf("Using fallback allocator\n");
    return (Allocator*)memory;
}

void fallback_allocator_destroy(Allocator* const allocator) {
    printf("Fallback allocator destroyed\n");
}

void* fallback_allocator_alloc(Allocator* const allocator, const size_t size) {
    printf("Fallback allocator allocating %zu bytes\n", size);
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void fallback_allocator_free(Allocator* const allocator, void* const memory) {
    printf("Fallback allocator freeing memory\n");
    munmap(memory, 0);
}

#define ARRAY_SIZE 200

void get_range(int index, int *start, int *end, int min_value, int max_value) {
    *start = (1 << (4 + index));
    *end = (1 << (5 + index)) - 1;
    if (*end > max_value) *end = max_value;
}

void fill_array(int *array, int size, int num_ranges, int min_value, int max_value) {
    int numbers_per_range = size / num_ranges;
    int index = 0;
    for (int range = 0; range < num_ranges; range++) {
        int start, end;
        get_range(range, &start, &end, min_value, max_value);
        for (int i = 0; i < numbers_per_range; i++) {
            array[index++] = start + rand() % (end - start + 1);
        }
    }
    while (index < size) {
        int start, end;
        get_range(rand() % num_ranges, &start, &end, min_value, max_value);
        array[index++] = start + rand() % (end - start + 1);
    }
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

double calculate_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char* argv[]) {
    allocator_create_t allocator_create = fallback_allocator_create;
    allocator_destroy_t allocator_destroy = fallback_allocator_destroy;
    allocator_alloc_t allocator_alloc = fallback_allocator_alloc;
    allocator_free_t allocator_free = fallback_allocator_free;

    void *library_handle = NULL; 

    if (argc > 1) {
        const char* library_path = argv[1];
        library_handle = dlopen(library_path, RTLD_LAZY);
        if (library_handle) {
            printf("Loaded library: %s\n", library_path);

            allocator_create = (allocator_create_t)dlsym(library_handle, "allocator_create");
            allocator_destroy = (allocator_destroy_t)dlsym(library_handle, "allocator_destroy");
            allocator_alloc = (allocator_alloc_t)dlsym(library_handle, "allocator_alloc");
            allocator_free = (allocator_free_t)dlsym(library_handle, "allocator_free");

            if (!allocator_create || !allocator_destroy || !allocator_alloc || !allocator_free) {
                printf("Failed to load symbols from library. Using fallback allocator.\n");
                allocator_create = fallback_allocator_create;
                allocator_destroy = fallback_allocator_destroy;
                allocator_alloc = fallback_allocator_alloc;
                allocator_free = fallback_allocator_free;
            }
        } else {
            printf("Failed to load library: %s. Using fallback allocator.\n", library_path);
        }
    } else {
        printf("No library path provided. Using fallback allocator.\n");
    }


    const size_t memory_size = 1024 * 1024; // 1 мб
    void* memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    Allocator* allocator = allocator_create(memory, memory_size);
    if (!allocator) {
        printf("Failed to create allocator\n");
        munmap(memory, memory_size);
        return 1;
    }

    // TEST
    int sizes[ARRAY_SIZE];
    srand(42);
    fill_array(sizes, ARRAY_SIZE, 8, 16, 2048);

    void *test[ARRAY_SIZE];

    struct timespec start, end;
    double time_used;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        test[i] = allocator_alloc(allocator, sizes[i]);
        if (!test[i]) {
            fprintf(stderr, "Error: allocation error.\n");
            return 1;
        }
    }
    for (int i = 0; i < ARRAY_SIZE; i++) {
        allocator_free(allocator, test[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    time_used = calculate_time(start, end);
    
    printf("Time of allocations and freeings: %f seconds\n", time_used);
    
    allocator_destroy(allocator);
    munmap(memory, memory_size);
    
    if (library_handle) {
        dlclose(library_handle);
    }

}
