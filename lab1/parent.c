#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

int read_str(char *str, size_t size) {
    ssize_t status = read(STDIN_FILENO, str, size - 1);
    if (status > 0) {
        if (str[status - 1] == '\n') {
            str[status - 1] = '\0';
        } else {
            str[status] = '\0'; 
            char ch;
            while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n');
        }
        return 0;
    }
    return -1;
}

int main() {   
    char filename[130];
    int pipe1[2], pipe2[2];

    {
        const char msg[] = "Enter the name of file:\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    if (read_str(filename, sizeof(filename)) != 0) {
        const char msg[] = "Buffer overflow\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
    }
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char msg[] = "Failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
    }

    const pid_t child = fork();

    if (child == -1)  {
        const char msg[] = "Failed to spawn new process\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
        
    } else if (child == 0) {   
        close(pipe1[1]);
        close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);

        char *const args[] = {"./child.out", filename, NULL};
        int status = execv("./child.out", args);
        if (status == -1) {
            const char msg[] = "Failed to exec into new exectuable image\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
		}

    } else {
        close(pipe2[1]);
        close(pipe1[0]);

        {
            const char msg[] = "Enter numbers. If you want to stop enter the prime number or Ctrl + D:\n";
            write(STDERR_FILENO, msg, sizeof(msg));
        }   

        while (1) {
            char buffer[256];
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
            
            if (bytes_read <= 0) break;
            
            write(pipe1[1], buffer, bytes_read);

            int res;
            ssize_t response = read(pipe2[0], &res, sizeof(res));
            if (response <= 0 || res == 1) { 
                break;
            }
        }

        close(pipe1[1]); 
        close(pipe2[0]);
    }

    return 0;
}