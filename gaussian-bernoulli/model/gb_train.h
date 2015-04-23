#ifndef __GB_TRAIN_H
#define __GB_TRAIN_H

#include <stdio.h>
#include <stdint.h>
#include "gb.h"
#include "../utils/utils.h"

double gb_grad(const gb_info_t *gb, gb_w_t *gradw, const double *V, const double *H, int numcase);
double gb_train(gb_info_t *gb, double learnrate, const double *V, const double *H, int numcase, int batchsize);
void check_gradient(gb_info_t *gb, const double *V, const double *H, int numcase);

#endif
