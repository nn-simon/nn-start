#include "../utils/random.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "sm_hid.h"

#define MAX_NODE 10000

static _simu_anneal(double *h, const double *w, const uint8_t *pos, const double *bh, int dim, int times)
{
	int nd, nl, nt;
	double esum = 0.0, emax = 0.0;
	double e, p;
	double _hmax[MAX_NODE];
	for (nt = 0; nt < times; nt++) {
		nd = (int)(1.0 * random_self() / RAND_SELF_MAX * dim);  
		e = bh[nd];
		for (nl = 0; nl < dim; nl++) {
			if (h[nl] < 0.5) // vecH[nl] == 0.0
				continue;
			if (pos[nd * dim + nl])
				e += w[nd * dim + nl];
			if (pos[nl * dim + nd])
				e += w[nl * dim + nd];
		}
		if (e >= 0.0) {
			if (h[nd] < 0.5) { // 0 -> 1
				esum += e;
				h[nd] = 1.0;
			} else {           // 1 -> 0|1
				p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
				h[nd] = random_self() > RAND_SELF_MAX * p ? 1.0 : 0.0;
				esum += (h[nd] - 1.0) * e;
			}
		} else {
			if (h[nd] < 0.5) { // 0 -> 0|1
				p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
				h[nd] = random_self() > RAND_SELF_MAX * p ? 1.0 : 0.0;
				esum += h[nd] * e;
			} else {           // 1 -> 0
				esum -= e;
				h[nd] = 0.0;
			}
		}
		if (esum > emax) {
			memcpy(_hmax, h, dim * sizeof(double));
			emax = esum;
		}
	}
	printf("_simu_annel: emax %lf\r", emax);
	memcpy(h, _hmax, dim * sizeof(double));
}

void sm_hid_sa(const sm_info_t *sm, const int *V, double *H, int numcase, int numsample, void *reserved)
{
	int nh, nc, ns, nl;
	double *bh = reserved;
	double *x0 = H;
	for (nc = 0; nc < numcase; nc++) {
		fprintf(stdout, "nc %d in sm_hid_sa\r", nc);
		const int *vecV = V + nc * sm->numvis;
		double *vecH = H + nc * sm->numhid;
		memcpy(bh, sm->w->bh, sm->numhid * sizeof(double));
		for (nh = 0; nh < sm->numhid; nh++) {
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				bh[nh] += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
		}
		memcpy(vecH, x0, sm->numhid * sizeof(double));
		//pr_array(stdout, vecH, 1, 18, 'd');
		_simu_anneal(vecH, sm->w->bm_w, sm->bm_pos, bh, sm->numhid, numsample * sm->numhid);
		//x0 = vecH;
		//pr_array(stdout, vecH, 1, 18, 'd');
	}
}
