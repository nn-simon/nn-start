#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"
#include "gb_train.h"

static void _pr_gb_info(gb_info_t *gb, char *msg)
{
	fprintf(stdout, "%s:\n\tnumvisXnumhid:%dx%d\n"
			"\tlambda:%lf\n"
			"\tlen_v2h_maxXclass_max:%dx%d\n", 
			msg, gb->numvis, gb->numhid, gb->lambda, gb->len_v2h_max, gb->class_max);
}

double gb_cost(gb_info_t *gb, gb_w_t *gradw, const double *V, const double *H, int numcase, int batchsize)
{
	int nb, nv;

	int total_batch = numcase / batchsize;
	double cost;
	fprintf(stdout, "+ vis:%d hid:%d bsize:%d learnrate: %lf +\n", 
		gb->numvis, gb->numhid, batchsize, learnrate);

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
		fprintf(stdout, "\tcost = %lf on %d batch\n", cost, nb);
	}

	return cost;
}

void check_gradient(gb_info_t *gb, const int *V, const double *H, int numcase)
{
	gb_w_t dw1, dw2, dw3;
	_pr_gb_info(gb, "dw1");
	construct_gb_w(gb, &dw1);
	_pr_gb_info(gb, "dw2");
	construct_gb_w(gb, &dw2);
	_pr_gb_info(gb, "dw3");
	construct_gb_w(gb, &dw3);
	double v0, vdx;
	v0 = gb_cost(gb, &dw1, V, H, numcase);
	printf("%lf\n", v0);
	int nv, nh, nl, nk, cnt = 0;
	double dx = 0.00001;
	for (nv = 0; nv < gb->numvis; nv++) {	
		for (nk = 0; nk < gb->numclass[nv]; nk++) {
			int len = gb->len_v2h[nv] + 1;
			for (nl = 0; nl < len; nl++) {
				gb->w->w[nv][nk * len + nl] += dx;
				vdx = gb_cost(gb, &dw2, V, H, numcase);
				printf("%d %lf|", cnt++, vdx);
				dw3.w[nv][nk * len + nl] = (vdx - v0) / dx;
				gb->w->w[nv][nk * len + nl] -= dx;
			}
			printf("\n");
		}
	}
	printf("%d\n", cnt);

	for (nh = 0; nh < gb->numhid; nh++) {
		gb->w->bh[nh] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase);
		printf("%d %lf|", cnt++, vdx);
		dw3.bh[nh] = (vdx - v0) / dx;
		gb->w->bh[nh] -= dx;
	}
	printf("\n%d\n", cnt);
	for (nl = 0; nl < gb->numhid * gb->numhid; nl++) {
		gb->w->bm_w[nl] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase);
		printf("%d %lf|", cnt++, vdx);
		dw3.bm_w[nl] = (vdx - v0) / dx;
		gb->w->bm_w[nl] -= dx;
	}
	printf("\n%d\n", cnt);
	gb_w_t *tp;
	tp = gb->w;
	gb->w = &dw1;
	out_w("gradw1.para", gb);
	gb->w = &dw3;
	out_w("gradw3.para", gb);
	gb->w = tp;
	destroy_gb_w(gb, &dw1);
	destroy_gb_w(gb, &dw2);
	destroy_gb_w(gb, &dw3);
}
