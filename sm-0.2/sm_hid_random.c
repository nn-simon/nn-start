#include "random.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "sm.h"

static _gibbs_sample(double *h, const double *w, const uint8_t *pos, const double *bh, int dim, int network_type)
{
	int nd, nl;
	double p;
	for (nd = 0; nd < dim; nd++) {
		p = bh[nd];
		if (network_type) {
			for (nl = 0; nl < dim; nl++) {
				if (h[nl] < 0.5) // vecH[nl] == 0.0
					continue;
				if (pos[nd * dim + nl])
					p += w[nd * dim + nl];
				if (pos[nl * dim + nd])
					p += w[nl * dim + nd];
			}
		}
		double tp1, tp2;
		tp1 = p;
		p = 1.0 / (1 + exp(p)); //p(h_j = 0 | h_{-j})
		tp2 = h[nd];
		h[nd] = random_self() > RAND_SELF_MAX * p ? 1.0 : 0.0;
//		if (nd < 17)
//			fprintf(stdout, "|%lf %lf %lf", tp1, p, bh[nd]);
//		fprintf(stdout, "(nd: %d %lf %d)\n", nd, p, (int)(h[nd] + 0.5));
	}
//	fprintf(stdout, "\n");
//	pr_array(stdout, h, 1, 17, 'd');
}

void sm_hid_random(const sm_info_t *sm, const int *V, double *H, int numcase, int numsample, void *reserved)
{
	int nh, nc, ns, nl;
	double *bh = reserved;
	double *x0 = H;
	for (nc = 0; nc < numcase; nc++) {
		fprintf(stdout, "nc %d in sm_hid_random\r", nc);
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
		//fprintf(stdout, "xx\n");
		//pr_array(stdout, vecH, 1, 20, 'd');
//		fprintf(stdout, "nc %d:\n", nc);
//		pr_array(stdout, vecH, 1, 17, 'd');
		for (ns = 0; ns < numsample; ns++)
			_gibbs_sample(vecH, sm->w->bm_w, sm->bm_pos, bh, sm->numhid, sm->type);
		//pr_array(stdout, vecH, 1, 20, 'd');
		x0 = vecH;
	}
}
