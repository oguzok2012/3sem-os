#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <sys/wait.h>
#include "../include.h"

typedef struct {
    int number;
    int stop_flag; // 1 — завершить, 0 — продолжать
} shared_data_t;

int main() {
    // Ввод имени файла
    char filename[128];
    printf("Enter the name of the output file: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        fprintf(stderr, "Failed to read file name\n");
        exit(EXIT_FAILURE);
    }
    filename[strcspn(filename, "\n")] = '\0'; // Удаление символа новой строки

    // Удаление существующего семафора и разделяемой памяти
    // sem_unlink(REQ_SEM_NAME);
    // sem_unlink(RES_SEM_NAME);
    // shm_unlink(SHM_NAME);

    // Создание разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Установка размера разделяемой памяти
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Маппинг разделяемой памяти
    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Создание семафоров
    sem_t *req_sem = sem_open(REQ_SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    sem_t *res_sem = sem_open(RES_SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (req_sem == SEM_FAILED || res_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Создание дочернего процесса
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Дочерний процесс
        execl("./child.out", "./child.out", filename, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    // Родительский процесс
    while (1) {
        int number;
        printf("Parent: Enter a number: ");
        if (scanf("%d", &number) != 1) break;
        // Очистка входного буфера
        while (getchar() != '\n');

        shared_data->number = number;
        shared_data->stop_flag = 0; // Сброс флага завершения
        printf("Parent set number to %d\n", shared_data->number);
        sem_post(req_sem); // Сигнал ребенку
        printf("Parent posted request semaphore\n");

        sem_wait(res_sem); // Ожидание сигнала от ребенка
        printf("Parent received response semaphore\n");
        if (shared_data->stop_flag) {
            printf("Parent: Received stop flag. Exiting...\n");
            break;
        }
    }

    // Ожидание завершения дочернего процесса
    pid_t status = waitpid(pid, NULL, 0);
    if (status == -1) perror("waitpid");

    // Очистка
    munmap(shared_data, sizeof(shared_data_t));
    sem_close(req_sem);
    sem_close(res_sem);
    sem_unlink(REQ_SEM_NAME);
    sem_unlink(RES_SEM_NAME);
    shm_unlink(SHM_NAME);
    return 0;
}