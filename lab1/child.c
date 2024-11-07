#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


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

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    while (1) {
        char buffer[256];
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        int number = atoi(buffer);
        int is_prime_ = is_prime(number);

        write(STDOUT_FILENO, &is_prime_, sizeof(is_prime_));
        
        if (is_prime_) break;
        write(fd, buffer, strlen(buffer));
    }
    
    const char term = '\0';
	write(fd, &term, sizeof(term));

    close(fd);
    return 0;
   
}