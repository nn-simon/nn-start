#include <math.h>
#include <stdio.h>
#include "sm_train.h"
#include "sm.h"

double hv_grad(const sm_info_t *sm, sm_w_t *gradw, const int *V, const double *H, int numcase)
{
	int nc, nl, nh;
	double cost = 0.0;

	for (nc = 0; nc < numcase; nc++) {
		const int *vecV = V + nc * sm->numvis;
		const double *vecH = H + nc * sm->numhid;
		for (nh = 0; nh < sm->numhid; nh++) {
			// bh part
			double e = sm->w->bh[nh];
			// softmax part
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				e += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
			// bm part
			if (sm->type) {
				for (nl = 0; nl < sm->numhid; nl++) {
					if (vecH[nl] < 0.5) // vecH[nl] == 0.0
						continue;
					if (sm->bm_pos[nh * sm->numhid + nl])
						e += sm->w->bm_w[nh * sm->numhid + nl];
					if (sm->bm_pos[nl * sm->numhid + nh])
						e += sm->w->bm_w[nl * sm->numhid + nh];
				}
			}
			//p(h_j|h_{-j}, V)
			double p = 1.0 / (1.0 + exp(-e));
			// grad: softmax part
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				//gradw->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]] += (vecH[nh] - p) / numcase;
				gradw->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]] += (vecH[nh] - p - sm->lambda * e) / numcase;
			}
			// grad: bm part
			if (sm->type) {
				for (nl = 0; nl < sm->numhid; nl++) {
					if (vecH[nl] < 0.5) // vecH[nl] == 0.0
						continue;
					if (sm->bm_pos[nh * sm->numhid + nl])
						gradw->bm_w[nh * sm->numhid + nl] += vecH[nh] - p - sm->lambda * e;
					if (sm->bm_pos[nl * sm->numhid + nh])
						gradw->bm_w[nl * sm->numhid + nh] += vecH[nh] - p - sm->lambda * e;
				}
			}
			//grad: bh part
			gradw->bh[nh] += vecH[nh] - p - sm->lambda * e;
			//cost += vecH[nh] * log(p) + (1 - vecH[nh]) * log(1 - p);
			if (vecH[nh] == 0)
				cost += log(1 - p);
			else
				cost += log(p);
			cost -= sm->lambda * e * e / 2.0;
		}
	}

	int ni;
	for (ni = 0; ni < sm->numhid; ni++)
		gradw->bh[ni] /= numcase;
	for (ni = 0; ni < sm->numhid * sm->numhid; ni++)
		gradw->bm_w[ni] /= numcase;
	return cost / numcase;
}
