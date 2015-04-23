#ifndef __UTILS_H
#define __UTILS_H
#include <unistd.h>
#include <stdio.h>

void pr_array(FILE *f, void *array, int numcase, int lencase, char flag);
void get_data(const char *name, void *data, size_t need_read);
void out_data(const char *name, void *data, int len);
void reorder(void *data, size_t size, const int *order, int numcase, int lencase);
void randperm(int *order, int numcase);

#endif
