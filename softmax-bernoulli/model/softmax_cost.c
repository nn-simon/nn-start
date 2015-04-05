#include <nag.h>
#include <nag_stdlib.h>
#include <nagf16.h>
#include <nagx04.h>
#include <math.h>
#include "sm_train.h"

static void vec_func(double *vec, int m, double (*func)(double))
{
	int i;
	for (i = 0; i < m; i++)
		vec[i] = func(vec[i]);
}

double softmax_cost(const double *w, double *gradw, const double *data, const int *label, int numclass, int lencase, int numcase, double *mem)
{
	double lambda = 0.0001;
	double *p = mem;
	NagError fail;
	INIT_FAIL(fail);
	nag_dgemm(Nag_RowMajor, Nag_NoTrans, Nag_Trans,
		numcase, numclass, lencase,
		1.0, data, lencase, w, lencase,
		0.0, p, numclass, &fail);
	if (fail.code != NE_NOERROR) {
		fprintf(stderr, "Error from dgemm!\n%s\n", fail.message);
	}
	vec_func(p, numcase * numclass, exp);
	double cost = 0.0;
	int nc;
	for (nc = 0; nc < numcase; nc++) {
		double *vec = p + nc * numclass;
		double sum = nag_dsum(numclass, vec, 1, &fail);
		nag_daxpby(numclass, 0.0, vec, 1, -1.0 / sum, vec, 1, &fail);
		cost += log(-vec[label[nc]]);
		vec[label[nc]] += 1;
	}
	cost /= numcase;
	nag_dgemm(Nag_RowMajor, Nag_Trans, Nag_NoTrans,
		numclass, lencase, numcase,
		1.0, p, numclass, data, lencase,
		0.0, gradw, lencase, &fail);
	if (fail.code != NE_NOERROR) {
		fprintf(stderr, "Error from dgemm!\n%s\n", fail.message);
	}
	nag_dgemm(Nag_RowMajor, Nag_Trans, Nag_NoTrans,
		1, 1, numclass * lencase,
		-lambda / 2, w, 1, w, 1,
		1.0, &cost, 1, &fail);
	if (fail.code != NE_NOERROR) {
		fprintf(stderr, "Error from dgemm!\n%s\n", fail.message);
	}

	int ni;
	for (ni = 0; ni < numclass * lencase; ni++)
		gradw[ni] /= numcase;

	nag_daxpby(numclass * lencase, -lambda, w, 1, 1.0, gradw, 1, &fail);
	return cost;
}
