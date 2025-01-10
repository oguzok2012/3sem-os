#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <sys/wait.h>
#include "communication.h"

static char CLIENT_PROGRAM_NAME[] = "./child.out";

int read_str(char *str, size_t size) {
    ssize_t status = read(STDIN_FILENO, str, size - 1);
    if (status > 0) {
        if (str[status - 1] == '\n') {
            str[status - 1] = '\0';
        } else {
            str[status] = '\0';
            char ch;
            while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n');
            return 1;
        }
        return 0;
    }
    return 1;
}


int main() {
    char filename[130];
    {
        const char msg[] = "Enter the name of file: ";
        write(STDIN_FILENO, msg, sizeof(msg));
    }
        
    if (read_str(filename, sizeof(filename)) != 0) {
        const char msg[] = "Failed to read file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    sem_unlink(REQ_SEM_NAME);
    sem_unlink(RES_SEM_NAME);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        const char msg[] = "Error: shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        const char msg[] = "Error: ftruncate\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        const char msg[] = "Error: mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE); 
    }

    sem_t *sem_to_child = sem_open(REQ_SEM_NAME, O_CREAT, 0666, 0);
    sem_t *sem_to_parent = sem_open(RES_SEM_NAME, O_CREAT, 0666, 0);
    
    if (sem_to_child == SEM_FAILED || sem_to_parent == SEM_FAILED) {
        const char msg[] = "Error: sem_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        const char msg[] = "Error: fork\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        int status = execl(CLIENT_PROGRAM_NAME, CLIENT_PROGRAM_NAME, filename, NULL);
        
        if (status == -1) {
            const char msg[] = "Error: execl\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }

    {
        const char msg[] = "Enter numbers. If you want to stop, enter the prime number or Ctrl + D:\n";
        write(STDIN_FILENO, msg, sizeof(msg));
    }

    shared_data->stop_flag = 0;

    while (!shared_data->stop_flag) {    

        char buffer[30];
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            shared_data->number = -1;
            sem_post(sem_to_child);
            break;
        }

        int number = atoi(buffer);

        shared_data->number = number;
        sem_post(sem_to_child);
        sem_wait(sem_to_parent);
    }
    
    pid_t status = waitpid(pid, NULL, 0);
    if (status == -1) {
        const char msg[] = "Error: waitpid\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    munmap(shared_data, sizeof(shared_data_t));
    sem_close(sem_to_child);
    sem_close(sem_to_parent);
    sem_unlink(REQ_SEM_NAME);
    sem_unlink(RES_SEM_NAME);
    shm_unlink(SHM_NAME);
    return 0;
}