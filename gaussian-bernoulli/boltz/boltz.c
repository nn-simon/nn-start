#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "boltz.h"
#include "../utils/random.h"

void rbm_min(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
{
	int nd;
	for (nd = 0; nd < numdim; nd++)
		x[nd] = bh[nd] > 0 ? 1.0 : 0.0;
}

void rbm_rand(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
{
	int nd;
	double p;
	for (nd = 0; nd < numdim; nd++) {
		p = 1.0 / (1 + exp(bh[nd]));
		x[nd] = random01() > p ? 1.0 : 0.0;
	}
}

static void _gibbs_sample(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh) // x must be initialized
{
	int nd, nl;
	double p;
	for (nd = 0; nd < numdim; nd++) {
		p = bh[nd];
		for (nl = 0; nl < numdim; nl++) {
			if (x[nl] < 0.5) // vecH[nl] == 0.0
				continue;
			if (pos[nd * numdim + nl])
				p += w[nd * numdim + nl];
			if (pos[nl * numdim + nd])
				p += w[nl * numdim + nd];
		}
		p = 1.0 / (1 + exp(p)); //p(h_j = 0 | h_{-j})
		x[nd] = random01() > p ? 1.0 : 0.0;
	}
}

void bm_rand(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
{
	int ns;
	for (ns = 0; ns < numsample; ns++)
		_gibbs_sample(x, numdim, w, pos, bh);
}

static double _simu_anneal(double *h, int dim, const double *w, const uint8_t *pos, const double *bh)
{
	int nl;
	int nd = (int)(random01() * dim);
	double tp = h[nd] > 0.5 ? 1.0 : 0.0;
	double e = bh[nd];
	for (nl = 0; nl < dim; nl++) {
		if (h[nl] < 0.5) // vecH[nl] == 0.0
			continue;
		if (pos[nd * dim + nl])
			e += w[nd * dim + nl];
		if (pos[nl * dim + nd])
			e += w[nl * dim + nd];
	}
	if (e >= 0.0) {
		if (h[nd] < 0.5) { // 0 -> 1
			h[nd] = 1.0;
		} else {           // 1 -> 0|1
			double p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
			h[nd] = random01() > p ? 1.0 : 0.0;
		}
	} else {
		if (h[nd] < 0.5) { // 0 -> 0|1
			double p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
			h[nd] = random01() > p ? 1.0 : 0.0;
		} else {           // 1 -> 0
			h[nd] = 0.0;
		}
	}
	return (h[nd] - tp) * e;
}

#define MAX_NODE 5000

void bm_min_sa(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
{
	int times = numsample * numdim;
	int nt;
	double esum = 0.0, emax = 0.0;
	double _xmax[MAX_NODE];
	for (nt = 0; nt < times; nt++) { 
		esum += _simu_anneal(x, numdim, w, pos, bh);
		if (esum > emax) {
			memcpy(_xmax, x, numdim * sizeof(double));
			emax = esum;
		}
	}
	memcpy(x, _xmax, numdim * sizeof(double));
}

//void rbm_min(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
//void rbm_min(double *x, int numdim, const double *w, const uint8_t *pos, const double *bh, int numsample, void *reserved)
