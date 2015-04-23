#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "../utils/random.h"
#include "../boltz/boltz.h"
#include "../model/gb.h"

void gb_rand(const gb_info_t *gb, void *ip, const double *V, double *H, int numcase, int numsample, void *reserved)
{
	int nh, nc, ns, nl;
	double *bh = reserved;
	double *x0 = H;
	for (nc = 0; nc < numcase; nc++) {
		fprintf(stdout, "nc %d in gb_hid_sa\r", nc);
		const double *vecV = V + nc * gb->numvis;
		double *vecH = H + nc * gb->numhid;
		memcpy(bh, gb->w->bh, gb->numhid * sizeof(double));
		for (nh = 0; nh < gb->numhid; nh++) {
			for (nl = 0; nl < gb->numvis; nl++)
				if (gb->vh_pos[nl * gb->numhid +nh])
					bh[nh] += gb->w->w[nl * gb->numhid + nh] * vecV[nl] / gb->w->sigma[nl];
		}
		memcpy(vecH, x0, gb->numhid * sizeof(double));
		//pr_array(stdout, vecH, 1, 18, 'd');
		bm_rand(vecH, gb->numhid, gb->w->bm_w, gb->bm_pos, bh, numsample, NULL);
		//x0 = vecH;
		//pr_array(stdout, vecH, 1, 18, 'd');
	}
}

