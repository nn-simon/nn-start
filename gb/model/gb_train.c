#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <nag.h>
#include <nagf16.h>
#include "gb.h"

static void _gradient_ascent(gb_info_t *gb, gb_w_t *dw, double epsilon)
{
	nag_daxpby(gb->numhid * gb->numvis, epsilon, dw->w, 1, 1.0, gb->w->w, 1, NULL);
	nag_daxpby(gb->numhid, epsilon, dw->bh, 1, 1.0, gb->w->bh, 1, NULL);
	nag_daxpby(gb->numvis, epsilon, dw->bv, 1, 1.0, gb->w->bv, 1, NULL);
	nag_daxpby(gb->numvis, epsilon, dw->sigma, 1, 1.0, gb->w->bv, 1, NULL);
	if (gb->type) 
		nag_daxpby(gb->numhid * gb->numhid, epsilon, dw->bm_w, 1, 1.0, gb->w->bm_w, 1, NULL);
}

double gb_train(gb_info_t *gb, double learnrate, const double *V, const double *H, int numcase, int batchsize)
{
	int nb, nv;

	int total_batch = numcase / batchsize;
	double cost;
	fprintf(stdout, "+ vis:%d hid:%d bsize:%d learnrate: %lf +\n", 
		gb->numvis, gb->numhid, batchsize, learnrate);
	gb_w_t gradw;
	construct_gb_w(gb, &gradw);

	for (nb = 0; nb < total_batch; nb++) {
		cost = 0.0;
		const double *H_batch = H + (long)nb * batchsize * gb->numhid;
		const double *V_batch = V + (long)nb * batchsize * gb->numvis;
		memset(gradw.w, 0, gb->numhid * gb->numvis * sizeof(double));
		memset(gradw.bh, 0, gb->numhid * sizeof(double));
		memset(gradw.bv, 0, gb->numvis * sizeof(double));
		memset(gradw.sigma, 0, gb->numvis * sizeof(double));
		memset(gradw.bm_w, 0, gb->numhid * gb->numhid * sizeof(double));
		cost = gb_grad(gb, &gradw, V_batch, H_batch, batchsize);
		_gradient_ascent(gb, &gradw, learnrate);
		fprintf(stdout, "\tcost = %lf on %d batch\n", cost, nb);
	}

	destroy_gb_w(gb, &gradw);
	return cost;
}
