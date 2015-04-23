#ifndef _RSM_H
#define _RSM_H
#include <stdint.h>

typedef struct {
	double **w;
	double *bh;
	double *bm_w;
} sm_w_t;

typedef struct {
	int numvis;
	int numhid;
	double lambda;
	int *numclass;
	int *position;
	int type; // type of network, rsm or sm
	sm_w_t *w;
	uint8_t *bm_pos;
	int *len_v2h;
	int **v2h;
	int *len_h2v; //how many nodes v are linked to the node h_i 
	int **h2v;    //which nodes v are linked to the node h_i
	int **pos_h2v;// where node h_i is in the array v2h[h2v[h_i][nl]]
	int len_v2h_max;
	int class_max;
} sm_info_t;

void construct_sm_w(sm_info_t *sm, sm_w_t *w);
void destroy_sm_w(sm_info_t *sm, sm_w_t *w);
void construct_sm(sm_info_t *sm, char *argv);
void destroy_sm(sm_info_t *sm);
void out_w(char *file, const sm_info_t *w);

#endif
