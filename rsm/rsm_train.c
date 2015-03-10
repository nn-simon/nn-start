#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rsm.h"
#include <nag.h>
#include <nagf16.h>

double rsm_train(double *w, double *bh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase, int batchsize)
{
	int nb, nv;
	int lenw_cur, lenw = 0;
	int mem_length = 0;

	for (nv = 0; nv < numvis; nv++) {
		lenw += numclass[nv] * (numhid + 1);
		mem_length += numclass[nv] * batchsize;
	}

	double *vh_gradw = (double *)malloc(lenw * sizeof(double));
	double *hv_gradw = (double *)malloc(lenw * sizeof(double));
	double *gradbh = (double *)malloc(numhid * sizeof(double));
	double *mem = (double *)malloc(mem_length * sizeof(double));
	if (!(vh_gradw && hv_gradw && mem && gradbh))
		fprintf(stderr, "alloc err in rsm_train!\n");

	double epsilonw = 0.05;
	double epsilonb = 0.05;
	int total_batch = numcase / batchsize;
	double cost = 0.0;

	int tpr = rand() % numvis;
	fprintf(stdout, "+ vis:%d hid:%d bsize:%d class[%d]:%d +\n", numvis, numhid, batchsize, tpr, numclass[tpr]);
	for (nb = 0; nb < total_batch; nb++) {
		cost = 0.0;
		lenw_cur = 0;
		memset(hv_gradw, 0, lenw * sizeof(double));
		memset(vh_gradw, 0, lenw * sizeof(double));
		memset(gradbh, 0, numhid * sizeof(double));
		const double *H_batch = H + nb * batchsize * (numhid + 1);
		const int *V_batch = V + nb * batchsize * numvis;
		for (nv = 0; nv < numvis; nv++) {
			cost += softmax_cost(w + lenw_cur, vh_gradw + lenw_cur, 
				H_batch, V_batch, batchsize, numhid + 1, numclass[nv], nv, numvis, mem);
			lenw_cur += numclass[nv] * (numhid + 1);
		}
		cost += hv_grad(w, hv_gradw, bh, gradbh,
				V_batch, H_batch, numvis, numhid, numclass, batchsize);
		nag_daxpby(lenw, epsilonw, vh_gradw, 1, 1.0, w, 1, NULL);
		nag_daxpby(lenw, epsilonw, hv_gradw, 1, 1.0, w, 1, NULL);
		nag_daxpby(numhid, epsilonb, gradbh, 1, 1.0, bh, 1, NULL);
		fprintf(stdout, "\tcost = %lf on %d batch\n", cost, nb);
	}
	free(vh_gradw);
	free(mem);
	free(gradbh);
	free(hv_gradw);
	return cost;
}
