#include <math.h>
#include <unistd.h>
#include <string.h>

void int_to_str(int num, char* str) {
    int index = 0;
    if (num == 0) {
        str[index++] = '0';
    } else {
        while (num > 0) {
            int digit = num % 10;
            str[index++] = digit + '0';
            num = num / 10;
        }
    }
    
    int start = 0;
    int end = index - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    str[index] = '\0';
}

void frac_to_str(float frac, int decimal_places, char* str) {
    for (int i = 0; i < decimal_places; i++) {
        frac *= 10.0;
        int digit = (int)frac;
        frac = frac - digit;
        str[i] = digit + '0';
    }
    str[decimal_places] = '\0';
}

void float_to_str(float number, char* buffer, int decimal_places) {
    int sign = 1;
    if (number < 0) {
        sign = -1;
        number = -number;
    }
    int integer_part = (int)floor(number);
    float fractional_part = number - integer_part;
    char int_str[20];
    char frac_str[20];

    int_to_str(integer_part, int_str);
    frac_to_str(fractional_part, decimal_places, frac_str);

    int int_len = 0;
    
    while (int_str[int_len] != '\0') int_len++;
    
    int frac_len = decimal_places;
    int total_len = int_len + frac_len + 1;
    
    if (sign == -1)
        total_len += 1;
    
    int index = 0;
    
    if (sign == -1)
        buffer[index++] = '-';

    for (int i = 0; i < int_len; i++)
        buffer[index++] = int_str[i];
    
    buffer[index++] = '.';
    
    for (int i = 0; i < frac_len; i++)
        buffer[index++] = frac_str[i];
    buffer[index] = '\0';
}


void print_time(double time) {
    char part1[] = "Sorting took ";
    char part2[30];
    float_to_str(time, part2, 4);
    char part3[] = " seconds\n";
    write(STDOUT_FILENO, part1, sizeof(part1));
    write(STDOUT_FILENO, part2, strlen(part2));
    write(STDOUT_FILENO, part3, sizeof(part3));
}