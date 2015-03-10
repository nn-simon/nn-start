#ifndef _RSM_H
#define _RSM_H

double hv_grad(const double *w, double *gradw, const double *bh, double *gradbh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase);
double rsm_train(double *w, double *bh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase, int batchsize);
double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numcase, int lencase, int numclass, int cur_pos, int numvis, double *mem);
void rsmhid_sample(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase);

#endif
