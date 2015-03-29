#ifndef __TRAIN_H
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

void get_data(const char *name,  void *data, size_t len);
void out_data(const char *name,  void *data, int len);
void pr_array(FILE *f, void *array, int numcase, int lencase, char flag);
void randperm(int *, int);
void reorder(void *, size_t, const int *, int, int);

typedef struct {
	int numcase;
	int numchannel;
	int channelcase;
	uint8_t *data;
	int *labels;
} data_info_t;


typedef struct {
	int iteration;
	int batchsize;
	int mininumcase;
	int nummix;
	double *mix;
} train_info_t;
#endif
