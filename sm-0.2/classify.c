#include "classify.h"
#include "train.h"
#include <nag.h>
#include <nag_stdlib.h>
#include <nagf16.h>
#include <nagx04.h>
#include <math.h>
#include <stdlib.h>

void clssfy_build(classify_t *clssfy)
{
	clssfy->mem = malloc(clssfy->numcase * clssfy->numclass * sizeof(double));
	clssfy->labels = malloc(clssfy->numcase * sizeof(int));
	clssfy->pred = malloc(clssfy->numcase * sizeof(int));
	if (!(clssfy->mem && clssfy->labels && clssfy->pred)) {
		fprintf(stderr, "malloc err in classfy_build\n");
		exit(0);
	}
}

void clssfy_clear(classify_t *clssfy)
{
	free(clssfy->mem);
	free(clssfy->labels);
	free(clssfy->pred);
}

void prediction(classify_t *clssfy)
{
	double *val = (double *)clssfy->mem;
	double max;
	NagError fail;
	INIT_FAIL(fail);
	//we must be careful, because the last point of any sample is always 1. rsm need it. We need exclude it.
	nag_dgemm(Nag_RowMajor, Nag_NoTrans, Nag_NoTrans,
		clssfy->numcase, clssfy->numclass, clssfy->lencase,
		1.0, clssfy->data, clssfy->lencase, clssfy->w, clssfy->numclass,
		0.0, val, clssfy->numclass, &fail);
	if (fail.code != NE_NOERROR) {
		fprintf(stderr, "Error from dgemm!\n%s\n", fail.message);
	}
//	fprintf(stdout, "xyxyxyxy\n");
//	pr_array(stdout, clssfy->data + 0 * (clssfy->lencase + 1), 1, 30, 'd');
//	pr_array(stdout, clssfy->data + 1 * (clssfy->lencase + 1), 1, 10, 'd');
//	pr_array(stdout, clssfy->data + 2 * (clssfy->lencase + 1), 1, 10, 'd');
//	pr_array(stdout, clssfy->data + 1000 * (clssfy->lencase + 1), 1, 10, 'd');
//	pr_array(stdout, clssfy->data + 1001 * (clssfy->lencase + 1), 1, 10, 'd');
//	pr_array(stdout, clssfy->data + 1002 * (clssfy->lencase + 1), 1, 10, 'd');
//	fprintf(stdout, "xyxyxyxywww\n");
//	pr_array(stdout, clssfy->w + 600 * clssfy->numclass, 5, clssfy->numclass, 'd');
//	fprintf(stdout, "xyxyxyxyval\n");
//	pr_array(stdout, val + 1000 * clssfy->numclass, 5, clssfy->numclass, 'd');
	int nc;
	for (nc = 0; nc < clssfy->numcase; nc++) {
		Integer imax;
		nag_dmax_val(clssfy->numclass, val + nc * clssfy->numclass, 1, &imax, &max, NULL);
		clssfy->pred[nc] = imax;
	}
}

void train(classify_t *clssfy)
{

}
