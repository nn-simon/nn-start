#include <math.h>
#include <stdio.h>
#include "train.h"

double hv_grad(const sm_info_t *sm, sm_w_t *gradw, const int *V, const double *H, int numcase)
{
	int nc, nl, nh;
	double cost = 0.0;

	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * sm->numvis;
		const double *vecH = H + nc * sm->numhid;
		for (nh = 0; nh < numhid; nh++) {
//			fprintf(f, "d: %d %d %lf\n", nc, nh, cost);
			// bh part
			double p = bh[nh];
			// softmax part
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				p += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
				//[lenw_cur + vecV[nv] * (numhid+1) + nh];
			}
			// bm part
			for (nl = 0; nl < sm->numhid; nl++) {
				if (vecH[nl] < 0.5) // vecH[nl] == 0.0
					continue;
				if (sm->bm_pos[nh][nl])
					p += sm->w->bm_w[nh][nl];
				if (sm->bm_pos[nl][nh])
					p += sm->w->bh_w[nl][nh];
			}
			//p(h_j|h_{-j}, V)
			p = 1.0 / (1.0 + exp(-p));
			// grad: softmax part
			for (nv = 0; nv < sm->numvis; nv++) {
				int id_v = sm->h2v[nh][nl];
				gradw->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]] += (vecH[nh] - p) / numcase;
			}
			// grad: bm part
			for (nl = 0; nl < sm->numhid; nl++) {
				if (vecH[nl] < 0.5) // vecH[nl] == 0.0
					continue;
				if (sm->bm_pos[nh][nl])
					gradw->bm_w[nh][nl] += vecH[nh] - p;
				if (sm->bm_pos[nl][nh])
					gradw->bh_w[nl][nh] += vecH[nh] - p;
			}
			//grad: bh part
			gradw->bh[nh] += vecH[nh] - p;
			//cost += vecH[nh] * log(p) + (1 - vecH[nh]) * log(1 - p);
			if (vecH[nh] == 0)
				cost += log(1 - p);
			else
				cost += log(p);
		}
	}

	int ni;
	for (ni = 0; ni < sm->numhid; ni++)
		gradw->bh[ni] /= numcase;
	for (ni = 0; ni < sm->numhid * sm->numhid; ni++)
		gradw->bm_w[ni] /= numcase;
	return cost / numcase;
}
