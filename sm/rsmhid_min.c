#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void rsmhid_min(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved)
{
	int nc, nv, nh;
	int lenw_cur;
	//double xxyy[1000];
	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * numvis;
		double *vecH = H + nc * numhid; /*there is a node whose value is always 1*/
		for (nh = 0; nh < numhid; nh++) {
			double p = bh[nh];
			lenw_cur = 0;
			for (nv = 0; nv < numvis; nv++) {
				p += w[lenw_cur + vecV[nv] * (numhid+1) + nh];
				lenw_cur += numclass[nv] * (numhid + 1);
			}
			p = 1.0 / (1.0 + exp(p)); // p(h_j = 0|v)
			// distortion metric: d(x, \hat{x}) = -log p(x, \hat{x})
			vecH[nh] = p >= 0.5 ? 0.0 : 1.0;
		}
	}
}
