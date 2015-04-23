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
#endif
