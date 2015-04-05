#ifndef __SM_TRAIN_H
#define __SM_TRAIN_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "sm.h"
#include "../utils/utils.h"

double hv_grad(const sm_info_t *sm, sm_w_t *gradw, const int *V, const double *H, int numcase);
double sm_train(sm_info_t *sm, double learnrate, const int *V, const double *H, int numcase, int batchsize);
double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numclass, int lencase, int numcase, double *mem);
void check_gradient(sm_info_t *sm, const int *V, const double *H, int numcase);

#endif
