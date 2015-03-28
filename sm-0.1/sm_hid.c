#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "sm.h"

static void _get_x(double *x0, const double *A, const double *c, int dim, void *mem)
{
	double *x1 = (double *)mem;
	double curvalue = 0.0;
	double lastvalue = 1.0;
	int row, col;

	while (fabs(curvalue - lastvalue) > 0.001) {
		//xAy^T + cx^T, known y
		memcpy(x1, c, dim * sizeof(double));
		for (col = 0; col < dim; col++) {
			if (x0[col] < 0.5) //x0[col] == 0.0
				continue;
			for (row = 0; row < dim; row++)
				x1[row] += A[row][col];
		}
		curvalue = 0.0;
		for (row = 0; row < dim; row++) {
			x0[row] = x1[row] > 0.0 ? 1.0 : 0.0;
			curvalue += x0[row] * x1[row];
		}
		//yAx^T + cx^T, known y
		memcpy(x1, c, dim * sizeof(double));
		for (row = 0; row < dim; row++) {
			if (x0[row] < 0.5) //x0[col] == 0.0
				continue;
			for (col = 0; col < dim; col++)
				x1[col] += A[row][col];
		}
		lastvalue = 0.0;
		for (row = 0; row < dim; row++) {
			x0[row] = x1[row] > 0.0 ? 1.0 : 0.0;
			lastvalue += x0[row] * x1[row];
		}
	}
}

void sm_hid(sm_info_t *sm, const int *V, double *H, int numcase, void *reserved)
{
	double *init = (double *)reserved;
	double *bh = (double *)malloc(sm->numhid * sizeof(double));
	double *mem = (double *)malloc(sm->numhid * sizeof(double));
	if (!(bh && mem)) {
		fprintf(stderr, "malloc err in sm_hid");
		exit(0);
	}
	int nc, nv, nh, nl;
	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * sm->numvis;
		double *vecH = H + nc * sm->numhid;
		for (nh = 0; nh < sm->numhid; nh++) {
			bh[nh] = sm->w->bh[nh];
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				bh[nh] += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
		}
		memcpy(vecH, init, sm->numhid * sizeof(double));
		_get_x(vecH, sm->w->bm_w, bh, sm->numhid, mem);
	}
	free(bh);
	free(mem);
}
