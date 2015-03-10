#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "random.h"

void rsmhid_sample(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase)
{
	int nc, nv, nh;
	int lenw_cur;
	//double xxyy[1000];
	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * numvis;
		double *vecH = H + nc * (numhid + 1); /*there is a node whose value is always 1*/
		for (nh = 0; nh < numhid; nh++) {
			double p = bh[nh];
			lenw_cur = 0;
			for (nv = 0; nv < numvis; nv++) {
				p += w[lenw_cur + vecV[nv] * (numhid+1) + nh];
				lenw_cur += numclass[nv] * (numhid + 1);
			}
	//		xxyy[nh] = p;
			p = 1.0 / (1.0 + exp(p));
			//vecH[nh] = rand() > p * RAND_MAX ? 1.0 : 0.0; //??????
			vecH[nh] = random_self() > p * RAND_SELF_MAX ? 1.0 : 0.0; //??????
		}
	//	int tp = random_self() % numhid;
	//	int tp = 5;
	//	printf("(%3d h[%3d]:%lf %lf %d)", nc, tp, xxyy[tp], 1.0 / (1 + exp(xxyy[tp])), (int)vecH[tp]);
		vecH[numhid] = 1.0;
	}
	//printf("\n");
}
