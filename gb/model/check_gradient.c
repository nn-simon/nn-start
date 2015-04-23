#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"

static void _pr_gb_info(gb_info_t *gb, char *msg)
{
	fprintf(stdout, "%s:\n\tnumvisXnumhid:%dx%d\n"
			"\tlambda:%lf\n",
			msg, gb->numvis, gb->numhid, gb->lambda);
}

static double gb_cost(gb_info_t *gb, gb_w_t *gradw, const double *V, const double *H, int numcase, int batchsize)
{
	int nb = 0, nv;

	int total_batch = numcase / batchsize;
	double cost;
	//fprintf(stdout, "+ vis:%d hid:%d bsize:%d +\n", 
	//	gb->numvis, gb->numhid, batchsize);

	cost = 0.0;
	const double *H_batch = H + (long)nb * batchsize * gb->numhid;
	const double *V_batch = V + (long)nb * batchsize * gb->numvis;
	memset(gradw->w, 0, gb->numhid * gb->numvis * sizeof(double));
	memset(gradw->bh, 0, gb->numhid * sizeof(double));
	memset(gradw->bv, 0, gb->numvis * sizeof(double));
	memset(gradw->sigma, 0, gb->numvis * sizeof(double));
	memset(gradw->bm_w, 0, gb->numhid * gb->numhid * sizeof(double));
	cost = gb_grad(gb, gradw, V_batch, H_batch, batchsize);
	//fprintf(stdout, "\tcost = %lf on %d batch\n", cost, nb);

	return cost;
}

void check_gradient(gb_info_t *gb, const double *V, const double *H, int numcase)
{
	gb_w_t dw1, dw2, dw3;
	_pr_gb_info(gb, "dw1");
	construct_gb_w(gb, &dw1);
	_pr_gb_info(gb, "dw2");
	construct_gb_w(gb, &dw2);
	_pr_gb_info(gb, "dw3");
	construct_gb_w(gb, &dw3);
	double v0, vdx;
	v0 = gb_cost(gb, &dw1, V, H, numcase, numcase);
	printf("%lf\nbv,sigma\n", v0);
	int nv, nh, nl, nk, cnt = 0;
	double dx = 0.00001;
	for (nv = 0; nv < gb->numvis; nv++) {
		gb->w->bv[nv] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase, numcase);
		gb->w->bv[nv] -= dx;
		dw3.bv[nv] = (vdx - v0) / dx;
		gb->w->sigma[nv] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase, numcase);
		gb->w->sigma[nv] -= dx;
		dw3.sigma[nv] = (vdx - v0) / dx;
		printf("%d %lf(%lf) %lf(%lf)|", cnt++, dw3.bv[nv], dw1.bv[nv], dw3.sigma[nv], dw1.sigma[nv]);
	}
	printf("\nbh\n");

	for (nh = 0; nh < gb->numhid; nh++) {
		gb->w->bh[nh] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase, numcase);
		gb->w->bh[nh] -= dx;
		dw3.bh[nh] = (vdx - v0) / dx;
		printf("%d %lf(%lf)|", cnt++, dw3.bh[nh], dw1.bh[nh]);
	}
	printf("\nbm_w\n");
	for (nl = 0; nl < gb->numhid * gb->numhid; nl++) {
		gb->w->bm_w[nl] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase, numcase);
		gb->w->bm_w[nl] -= dx;
		dw3.bm_w[nl] = (vdx - v0) / dx;
		printf("%d %lf(%lf)|", cnt++, dw3.bm_w[nl], dw1.bm_w[nl]);
	}
	printf("\nvh\n");
	for (nl = 0; nl < gb->numhid * gb->numvis; nl++) {
		gb->w->w[nl] += dx;
		vdx = gb_cost(gb, &dw2, V, H, numcase, numcase);
		gb->w->w[nl] -= dx;
		dw3.w[nl] = (vdx - v0) / dx;
		printf("%d %lf(%lf)|", cnt++, dw3.w[nl], dw1.w[nl]);
	}
	printf("\n");
	double diff = 0.0, maxerr =0.0, err;
	for (nl = 0; nl < gb->numhid; nl++) {
		err = dw1.bh[nl] - dw3.bh[nl];
		err = err * err;
		if (maxerr < err)
			maxerr = err;
		diff += err;
	}
	printf("bh:%lf %lf\n", maxerr, diff);

	maxerr = 0.0;
	for (nl = 0; nl < gb->numvis; nl++) {
		err = dw1.bv[nl] - dw3.bv[nl];
		err = err * err;
		if (maxerr < err)
			maxerr = err;
		diff += err;
	}
	printf("bv:%lf %lf\n", maxerr, diff);

	maxerr = 0.0;
	for (nl = 0; nl < gb->numvis; nl++){
		err = dw1.sigma[nl] - dw3.sigma[nl];
		err = err * err;
		if (maxerr < err)
			maxerr = err;
		diff += err;
	}
	printf("sigma:%lf %lf\n", maxerr, diff);

	maxerr = 0.0;
	for (nl = 0; nl < gb->numhid * gb->numvis; nl++) {
		err = dw1.w[nl] - dw3.w[nl];
		err = err * err;
		if (maxerr < err)
			maxerr = err;
		diff += err;
	}
	printf("w:%lf %lf\n", maxerr, diff);

	maxerr = 0.0;
	for (nl = 0; nl < gb->numhid * gb->numhid; nl++) {
		err = dw1.bm_w[nl] - dw3.bm_w[nl];
		err = err * err;
		if (maxerr < err)
			maxerr = err;
		diff += err;
	}
	printf("bm_w:%lf %lf\n", maxerr, diff);
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
