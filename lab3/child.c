#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include <sys/types.h>
#include "communication.h"


int is_prime(int number) {
    if (number < 2) return 0;
    for (int i = 2; i * i <= number; i++) {
        if (number % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        const char msg[] = "At least one argument is required\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    const char *output_file = argv[1];

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        const char msg[] = "Error: shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        const char msg[] = "Error: mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE); 
    }

    sem_t *sem_to_child = sem_open(REQ_SEM_NAME, 0);
    sem_t *sem_to_parent = sem_open(RES_SEM_NAME, 0);
    if (sem_to_child == SEM_FAILED || sem_to_parent == SEM_FAILED) {
        const char msg[] = "Error: sem_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        const char msg[] = "Error: open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem_to_child);
        int number = shared_data->number;

        if (number < 0 || is_prime(number)) {
            shared_data->stop_flag = 1;
            sem_post(sem_to_parent);
            break;
        }
    
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d\n", number);
        write(fd, buffer, strlen(buffer));
    
        shared_data->stop_flag = 0;
        sem_post(sem_to_parent);
    }

    close(fd);
    munmap(shared_data, sizeof(shared_data_t));
    sem_close(sem_to_child);
    sem_close(sem_to_parent);
    return 0;
}