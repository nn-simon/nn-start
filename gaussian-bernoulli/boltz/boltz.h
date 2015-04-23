#ifndef __BOLTZ_H
#define __BOLTZ_H
#include <stdint.h>

typedef void (*bm_hid)(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved);
void rbm_min(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved);
void rbm_rand(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved);
void bm_rand(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved);
void bm_min_sa(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved);
#endif 
