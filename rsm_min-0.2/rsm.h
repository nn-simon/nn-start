#ifndef _RSM_H
#define _RSM_H

double hv_grad(const double *w, double *gradw, const double *bh, double *gradbh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase);
double rsm_train(double *w, double *bh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase, int batchsize);
double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numcase, int lencase, int numclass, int cur_pos, int numvis, double *mem);
typedef void (*func_get_hid)(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void rsmhid_min(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void classify_get_hid(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void init_hid_nag_ip_struct(int num_var, int num_cons);
void free_hid_nag_ip_struct();

#endif
