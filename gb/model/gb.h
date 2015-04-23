#ifndef _GB_H
#define _GB_H
#include <stdint.h>

typedef struct {
	double *w;
	double *sigma;
	double *bv;
	double *bh;
	double *bm_w;
} gb_w_t;

typedef struct {
	int numvis;
	int numhid;
	double lambda;
	int type; // type of network, rsm or sm
	gb_w_t *w;
	uint8_t *bm_pos;
	uint8_t *vh_pos;
} gb_info_t;

void check_gradient(gb_info_t *gb, const double *V, const double *H, int numcase);
void construct_gb_w(gb_info_t *gb, gb_w_t *w);
void destroy_gb_w(gb_info_t *gb, gb_w_t *w);
void construct_gb(gb_info_t *gb, char *argv);
void destroy_gb(gb_info_t *gb);
void out_w(char *file, const gb_info_t *gb);
double gb_grad(const gb_info_t *gb, gb_w_t *gradw, const double *V, const double *H, int numcase);
double gb_train(gb_info_t *gb, double learnrate, const double *V, const double *H, int numcase, int batchsize);

#endif
