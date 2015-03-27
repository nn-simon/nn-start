#ifndef _RSM_H
#define _RSM_H

typedef struct {
	double **w;
	double *bh;
	double *bm_w;
} sm_w_t;

typedef struct {
	int numvis;
:q
	int numhid;
	int *numclass;
	int *position;
	sm_w_t *w;
	uint8_t *bm_pos;
	int *len_v2h;
	int **v2h;
	int *len_h2v; //how many nodes v are linked to the node h_i 
	int **h2v;    //which nodes v are linked to the node h_i
	int **pos_h2v;// where node h_i is in the array v2h[h2v[h_i][nl]]
	int len_v2h_max;
} sm_info_t;

double hv_grad(const double *w, double *gradw, const double *bh, double *gradbh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase);
double rsm_train(double *w, double *bh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase, int batchsize);
double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numcase, int lencase, int numclass, int cur_pos, int numvis, double *mem);
typedef void (*func_get_hid)(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void rsmhid_min(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void classify_get_hid(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved);
void init_hid_nag_ip_struct(int num_var, int num_cons);
void free_hid_nag_ip_struct();

#endif
