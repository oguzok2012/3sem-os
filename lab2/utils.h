#ifndef UTILS_H
#define UTILS_H

#include <math.h>

void int_to_str(int num, char* str);
void frac_to_str(float frac, int decimal_places, char* str);
void float_to_str(float number, char* buffer, int decimal_places);
void print_time(double time);

#endif // UTILS_H