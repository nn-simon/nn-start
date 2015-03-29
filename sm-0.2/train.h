#ifndef __TRAIN_H
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "sm.h"

void get_data(const char *name,  void *data, size_t len);
void out_data(const char *name,  void *data, int len);
void pr_array(FILE *f, void *array, int numcase, int lencase, char flag);
void randperm(int *, int);
void reorder(void *, size_t, const int *, int, int);
double hv_grad(const sm_info_t *sm, sm_w_t *gradw, const int *V, const double *H, int numcase);
double sm_train(sm_info_t *sm, const int *V, const double *H, int numcase, int batchsize);
double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numclass, int lencase, int numcase, double *mem);
void check_gradient(sm_info_t *sm, const int *V, const double *H, int numcase);

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
