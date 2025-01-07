#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <sys/types.h>
#include "../include.h"

typedef struct {
    int number;
    int stop_flag; // 1 — завершить, 0 — продолжать
} shared_data_t;

int is_prime(int number) {
    if (number < 2) return 0;
    for (int i = 2; i * i <= number; i++) {
        if (number % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *output_file = argv[1];

    // Открытие разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Маппинг разделяемой памяти
    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Открытие семафоров
    sem_t *req_sem = sem_open(REQ_SEM_NAME, 0);
    sem_t *res_sem = sem_open(RES_SEM_NAME, 0);
    if (req_sem == SEM_FAILED || res_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Открытие файла для записи
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(req_sem); // Ожидание сигнала от родителя
        printf("Child received request semaphore\n");
        int number = shared_data->number;

        if (number < 0 || is_prime(number)) {
            shared_data->stop_flag = 1; // Установка флага завершения
            sem_post(res_sem); // Сигнал родителю
            printf("Child exiting\n");
            break;
        } else {
            // Запись составного числа в файл
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d\n", number);
            write(fd, buffer, strlen(buffer));
            printf("Child: Wrote number %d to file\n", number);

            shared_data->stop_flag = 0; // Сброс флага завершения
            sem_post(res_sem); // Сигнал родителю
            printf("Child posted response semaphore\n");
        }
    }

    close(fd);
    munmap(shared_data, sizeof(shared_data_t));
    sem_close(req_sem);
    sem_close(res_sem);
    return 0;
}