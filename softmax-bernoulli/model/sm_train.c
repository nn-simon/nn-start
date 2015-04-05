#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <nag.h>
#include <nagf16.h>
#include "sm.h"
#include "sm_train.h"

static void _get_data_from_H(double *data, const double *H, int *v2h, int len_v2h, int lencase, int numcase)
{
	int nc, nl;
	for (nc = 0; nc < numcase; nc++) {
		for (nl = 0; nl < len_v2h; nl++)
			data[nc * (len_v2h + 1) + nl] = H[nc * lencase + v2h[nl]];
		data[nc * (len_v2h + 1) + len_v2h] = 1.0;
	}
}

static void _get_label_from_V(int *label, const int *V, int pos, int lencase, int numcase)
{
	int nc;
	for (nc = 0; nc < numcase; nc++)
		label[nc] = V[pos + nc * lencase];
}

static void _gradient_ascent(sm_info_t *sm, sm_w_t *dw, double epsilon)
{
	int nv;
	for (nv = 0; nv < sm->numvis; nv++)
		nag_daxpby(sm->numclass[nv] * (sm->len_v2h[nv] + 1), epsilon, dw->w[nv], 1, 1.0, sm->w->w[nv], 1, NULL);
	nag_daxpby(sm->numhid, epsilon, dw->bh, 1, 1.0, sm->w->bh, 1, NULL);
	if (sm->type) 
		nag_daxpby(sm->numhid * sm->numhid, epsilon, dw->bm_w, 1, 1.0, sm->w->bm_w, 1, NULL);
}

double sm_train(sm_info_t *sm, double learnrate, const int *V, const double *H, int numcase, int batchsize)
{
	int nb, nv;

	int total_batch = numcase / batchsize;
	double cost;
	int tpr = rand() % sm->numvis;
	fprintf(stdout, "+ vis:%d hid:%d bsize:%d class[%d]:%d learn rate: %lf +\n", 
		sm->numvis, sm->numhid, batchsize, tpr, sm->numclass[tpr], learnrate);
	//for softmax_cost
	double *data = (double *)malloc(batchsize * (sm->len_v2h_max + 1) * sizeof(double));
	double *mem = (double *)malloc(batchsize * sm->class_max * sizeof(double));
	int *label = (int *)malloc(batchsize * sizeof(double));
	if (!(data && mem && label)) {
		fprintf(stderr, "alloc err in sm_train!\n");
		exit(0);
	}
	
	//for grad
	sm_w_t gradw;
	construct_sm_w(sm, &gradw);

	for (nb = 0; nb < total_batch; nb++) {
		cost = 0.0;
		const double *H_batch = H + nb * batchsize * sm->numhid;
		const int *V_batch = V + nb * batchsize * sm->numvis;
		for (nv = 0; nv < sm->numvis; nv++) {
			_get_data_from_H(data, H_batch, sm->v2h[nv], sm->len_v2h[nv], sm->numhid, batchsize);
			_get_label_from_V(label, V_batch, nv, sm->numvis, batchsize);
			cost += softmax_cost(sm->w->w[nv], gradw.w[nv], 
				data, label, sm->numclass[nv], sm->len_v2h[nv] + 1, batchsize, mem);
		}
		memset(gradw.bh, 0, sm->numhid * sizeof(double));
		memset(gradw.bm_w, 0, sm->numhid * sm->numhid * sizeof(double));
		cost += hv_grad(sm, &gradw, V_batch, H_batch, batchsize);
		_gradient_ascent(sm, &gradw, learnrate);
		fprintf(stdout, "\tcost = %lf on %d batch\n", cost, nb);
	}

	destroy_sm_w(sm, &gradw);
	free(mem);
	free(data);
	free(label);
	return cost;
}
