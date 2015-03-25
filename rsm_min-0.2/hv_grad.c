#include <math.h>
#include <stdio.h>

static void pr_array(FILE *f, void *array, int numcase, int lencase, char flag)
{
	union value {
		void *v;
		int *i;
		double *d;
	} v;
	v.v = array;
	if (flag == 'i'){
	int row, col;
	for (row = 0; row < numcase; row++) {
		for (col = 0; col < lencase; col++)
			fprintf(f, "%d ", v.i[row * lencase + col]);
		fprintf(f, "\n");
	}
	}
	if (flag == 'd'){
	int row, col;
	for (row = 0; row < numcase; row++) {
		for (col = 0; col < lencase; col++)
			fprintf(f, "%lf ", v.d[row * lencase + col]);
		fprintf(f, "\n");
	}
	}
}

double hv_grad(const double *w, double *gradw, const double *bh, double *gradbh, const int *V, const double *H, int numvis, int numhid, const int *numclass, int numcase)
{
	int nc, nv, nh;
	int lenw_cur;
	double cost = 0.0;

	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * numvis;
		const double *vecH = H + nc * (numhid + 1); // there is a node whose value is always 1
		for (nh = 0; nh < numhid; nh++) {
//			fprintf(f, "d: %d %d %lf\n", nc, nh, cost);
			double p = bh[nh];
			lenw_cur = 0;
			for (nv = 0; nv < numvis; nv++) {
				p += w[lenw_cur + vecV[nv] * (numhid+1) + nh];
				lenw_cur += numclass[nv] * (numhid + 1);
			}
			p = 1.0 / (1.0 + exp(-p));
			lenw_cur = 0;
			for (nv = 0; nv < numvis; nv++) {
				gradw[lenw_cur + vecV[nv] * (numhid + 1) + nh] += vecH[nh] - p;
				lenw_cur += numclass[nv] * (numhid + 1);
			}
			//cost += vecH[nh] * log(p) + (2 - vecH[nh]) * log(1 - p);
			if (vecH[nh] == 0)
				cost += log(1 - p);
			else
				cost += log(p);
			gradbh[nh] += vecH[nh] - p;
		}
	}

	int ni;
	for (ni = 0; ni < lenw_cur; ni++)
		gradw[ni] /= numcase;
	for (ni = 0; ni < numhid; ni++)
		gradbh[ni] /= numcase;
	return cost / numcase;
}
