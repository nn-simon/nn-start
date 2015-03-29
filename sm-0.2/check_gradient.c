#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sm.h"

static void _get_data_from_H(double *data, const double *H, int *v2h, int len_v2h, int lencase, int numcase)
{
	int nc, nl;
	for (nc = 0; nc < numcase; nc++) {
		for (nl = 0; nl < len_v2h; nl++)
			data[nc * (len_v2h + 1) + nl] = H[nc * lencase + v2h[nl]];
		data[nc * (len_v2h + 1) + len_v2h] = 1.0;
	}
}

static void _pr_sm_info(sm_info_t *sm, char *msg)
{
	fprintf(stdout, "%s:\n\tnumvisXnumhid:%dx%d\n"
			"\tlearnrate:%lf\n"
			"\tlen_v2h_maxXclass_max:%dx%d\n", 
			msg, sm->numvis, sm->numhid, sm->learnrate, sm->len_v2h_max, sm->class_max);
}

static void _get_label_from_V(int *label, const int *V, int pos, int lencase, int numcase)
{
	int nc;
	for (nc = 0; nc < numcase; nc++)
		label[nc] = V[pos + nc * lencase];
}

static double sm_cost(sm_info_t *sm, sm_w_t *gradw, const int *V, const double *H, int numcase)
{
	int nb, nv;
	double cost = 0.0;
	//int tpr = rand() % sm->numvis;
	//fprintf(stdout, "+ vis:%d hid:%d bsize:%d class[%d]:%d learn rate: %lf +\n", 
	//	sm->numvis, sm->numhid, numcase, tpr, sm->numclass[tpr], sm->learnrate);
	//for softmax_cost
	double *data = (double *)malloc(numcase * (sm->len_v2h_max + 1) * sizeof(double));
	double *mem = (double *)malloc(numcase * sm->class_max * sizeof(double));
	int *label = (int *)malloc(numcase * sizeof(double));
	if (!(data && mem && label)) {
		fprintf(stderr, "alloc err in sm_train!\n");
		exit(0);
	}
	
	//for grad
	for (nv = 0; nv < sm->numvis; nv++) {
		_get_data_from_H(data, H, sm->v2h[nv], sm->len_v2h[nv], sm->numhid, numcase);
		_get_label_from_V(label, V, nv, sm->numvis, numcase);
		cost += softmax_cost(sm->w->w[nv], gradw->w[nv], 
			data, label, sm->numclass[nv], sm->len_v2h[nv] + 1, numcase, mem);
	}
	memset(gradw->bh, 0, sm->numhid * sizeof(double));
	memset(gradw->bm_w, 0, sm->numhid * sm->numhid * sizeof(double));
	cost += hv_grad(sm, gradw, V, H, numcase);

	free(mem);
	free(data);
	free(label);
	return cost;
}

void check_gradient(sm_info_t *sm, const int *V, const double *H, int numcase)
{
	sm_w_t dw1, dw2, dw3;
	_pr_sm_info(sm, "dw1");
	construct_sm_w(sm, &dw1);
	_pr_sm_info(sm, "dw2");
	construct_sm_w(sm, &dw2);
	_pr_sm_info(sm, "dw3");
	construct_sm_w(sm, &dw3);
	double v0, vdx;
	v0 = sm_cost(sm, &dw1, V, H, numcase);
	printf("%lf\n", v0);
	int nv, nh, nl, nk, cnt = 0;
	double dx = 0.00001;
	for (nv = 0; nv < sm->numvis; nv++) {	
		for (nk = 0; nk < sm->numclass[nv]; nk++) {
			int len = sm->len_v2h[nv] + 1;
			for (nl = 0; nl < len; nl++) {
				sm->w->w[nv][nk * len + nl] += dx;
				vdx = sm_cost(sm, &dw2, V, H, numcase);
				printf("%d %lf|", cnt++, vdx);
				dw3.w[nv][nk * len + nl] = (vdx - v0) / dx;
				sm->w->w[nv][nk * len + nl] -= dx;
			}
			printf("\n");
		}
	}
	printf("%d\n", cnt);

	for (nh = 0; nh < sm->numhid; nh++) {
		sm->w->bh[nh] += dx;
		vdx = sm_cost(sm, &dw2, V, H, numcase);
		printf("%d %lf|", cnt++, vdx);
		dw3.bh[nh] = (vdx - v0) / dx;
		sm->w->bh[nh] -= dx;
	}
	printf("\n%d\n", cnt);
	for (nl = 0; nl < sm->numhid * sm->numhid; nl++) {
		sm->w->bm_w[nl] += dx;
		vdx = sm_cost(sm, &dw2, V, H, numcase);
		printf("%d %lf|", cnt++, vdx);
		dw3.bm_w[nl] = (vdx - v0) / dx;
		sm->w->bm_w[nl] -= dx;
	}
	printf("\n%d\n", cnt);
	sm_w_t *tp;
	tp = sm->w;
	sm->w = &dw1;
	out_w("gradw1.para", sm);
	sm->w = &dw3;
	out_w("gradw3.para", sm);
	sm->w = tp;
	destroy_sm_w(sm, &dw1);
	destroy_sm_w(sm, &dw2);
	destroy_sm_w(sm, &dw3);
}
